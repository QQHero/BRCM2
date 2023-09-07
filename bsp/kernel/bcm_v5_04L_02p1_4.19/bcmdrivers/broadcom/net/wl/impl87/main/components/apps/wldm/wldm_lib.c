/*
 * Broadcom Wifi Data Model Library
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/file.h>	/* For flock() */
#include <sys/types.h>	/* For getpid() */
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "wlcsm_defs.h"
#include "wlcsm_lib_api.h"
#include "wlioctl.h"
#include "common_utils.h"
#include "shutils.h"

#include "wldm_lib.h"

extern char *__progname;
unsigned long wldm_msglevel = WLDM_DEBUG_ERROR;
#define WLDM_LOG_FILE	"/rdklogs/logs/wifi_vendor_hal.log"

void
wldm_log(const char *fmt, ...)
{
	va_list args;
#ifdef CMWIFI_RDKB
	FILE *log_fd = NULL;
	time_t ltime;
	char buf[STRING_LENGTH_32];
	int len;

	log_fd = fopen(WLDM_LOG_FILE, "a");
	if (log_fd) {
		ltime = time(NULL);
		asctime_r(localtime(&ltime), buf);
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		va_start(args, fmt);
		fprintf(log_fd, "%s : ", buf);
		vfprintf(log_fd, fmt, args);
		va_end(args);
		fflush(log_fd);
		fclose(log_fd);
	}
#else
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	fflush(stdout);
#endif
	return;
}

#define DEFAULT_RADIO_IFNAMES		radio_nvifname
#if (MAX_WLAN_ADAPTER > 3) || (MAX_WLAN_ADAPTER < 1)
#error "MAX_WLAN_ADAPTER is out of range, must be 1 ~ 3."
#endif /* MAX_WLAN_ADAPTER */

#if (MAX_WLAN_ADAPTER >= 3)
#define DEFAULT_RUMTIME_RADIOS		3
#define DEFAULT_IFNAMES			bss_x3_nvifname
#elif (MAX_WLAN_ADAPTER == 2)
#define DEFAULT_RUMTIME_RADIOS		2
#define DEFAULT_IFNAMES			bss_x2_nvifname
#else
#define DEFAULT_RUMTIME_RADIOS		1
#define DEFAULT_IFNAMES			bss_x1_nvifname
#endif  /* MAX_WLAN_ADAPTER */

#define WL_MAX_BSSIDS			(MAX_WLAN_ADAPTER * WL_MAX_NUM_SSID)
#define WL_MAX_ACS			(MAX_WLAN_ADAPTER * WL_MAX_NUM_SSID * AC_COUNT)
/* wl bssidx of a radio */
#define BSS_IDX(apIndex)		((apIndex < (2 * WL_MAX_NUM_SSID)) ? \
	(apIndex / 2) : (apIndex % WL_MAX_NUM_SSID))
#define RADIO_INDEX(apIndex)		((apIndex < (2 * WL_MAX_NUM_SSID)) ? \
	(apIndex % 2) : (apIndex / WL_MAX_NUM_SSID))

#define APINDEX_CHECK(apIndex, retval) \
do { \
	if ((apIndex) >= max_num_of_aps || (apIndex) < 0) { \
		WIFI_DBG("%s: ap index %d out of range!\n", __FUNCTION__, apIndex); \
		return retval; \
	} \
}	while (0)

#define RADIOINDEX_CHECK(radioIndex, retval) \
do { \
	if ((radioIndex) >= number_of_radios || (radioIndex) < 0) { \
		WIFI_DBG("%s: radio index %d out of range!\n", __FUNCTION__, radioIndex); \
		return retval; \
	} \
}	while (0)

/* The action to be taken */
#define ACTION_SYS_RESTART		(1 << 0)	/* Call rc restart the whole subsystem */
#define ACTION_DRV_WLCONF		(1 << 1)	/* Call wlconf to reconfigure the wl IFs */
#define ACTION_APP_NVRAM_COMMIT		(1 << 2)	/* Commit nvram values to /data/nvram */
#define ACTION_APP_ALL_DAEMONS		(1 << 3)	/* Reload/signal all daemons */
#define ACTION_APP_ACSD			(1 << 4)	/* Reload/signal to acsd */
#define ACTION_APP_HOSTAPD		(1 << 5)	/* Start/restart hostapd or nas+wps */
#define ACTION_APP_HOSTAPD_STOP		(1 << 6)	/* Stop hostapd or nas+wps */

#if (WLDM_ACTION_WAIT_TIME_MS > 0)
#define WLDM_DEFER_TIMER	1
#endif /* WLDM_ACTION_WAIT_TIME_MS > 0 */

#if (WLDM_AUTO_APPLY_TIME_MS > 0)
#define WLDM_APPLY_TIMER	1
#endif /* WLDM_APPLY_WAIT_TIME_MS */

/* Multiple timers support with thread protection in a process.
 * A (timer-scheduling) thread can start a timer by its timer_info. When the timer expires,
 * a (timeout-handling) thread is created by linux to execute the given function.
 */
typedef struct _timer_info {
	/* Used by timer-scheduler to arm timer */
	pthread_mutex_t		lock;
	int			repeat_count;
	timer_t			timerid;
	void			*arg;
	int			(*func)(void *);	/* The timeout-handler function */
}	timer_info;

/* Mutex to protect actions_to_defer, deferred_actions[] and all objects */
static pthread_mutex_t wldm_cntxt_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef WLDM_MAX_THREADS
#if (WLDM_MAX_THREADS <= 1)
#error "WLDM_MAX_THREADS is not valid!"
#endif /* WLDM_MAX_THREADS */

typedef struct wldm_cntx_s {
	pid_t								tid;			/* Thread/process ID */
	int								actions_to_defer;
									/* Tell wldm_apply functions to defer the marked actions */
	int								deferred_actions[MAX_WLAN_ADAPTER];
									/* Let wldm_apply functions to mark the deferred actions,
									*  which will be done later at appropriate time
									*/
#if defined(WLDM_APPLY_TIMER)
	timer_info							apply_timerinfo;	/* Auto apply timer */
#endif /* WLDM_APPLY_TIMER */

	Radio_Object							*objRadio[MAX_WLAN_ADAPTER];
	X_BROADCOM_COM_Radio_Object					*objX_BROADCOM_COM_Radio[MAX_WLAN_ADAPTER];
	X_RDK_Radio_Object						*objX_RDK_Radio[MAX_WLAN_ADAPTER];
	SSID_Object							*objSSID[WL_MAX_BSSIDS];
	X_LGI_Rates_Bitmap_Control_Object				*objX_LGI_RatesBitmap[WL_MAX_BSSIDS];
	AccessPoint_Object						*objAccessPoint[WL_MAX_BSSIDS];
	X_RDK_AccessPoint_Object					*objX_RDK_AccessPoint[WL_MAX_BSSIDS];
	X_BROADCOM_COM_AccessPoint_Object				*objX_BROADCOM_COM_AccessPoint[WL_MAX_BSSIDS];
	AccessPoint_Security_Object					*objAccessPoint_Security[WL_MAX_BSSIDS];
	X_RDK_AccessPoint_Security_Object				*objX_RDK_AccessPoint_Security[WL_MAX_BSSIDS];
	AccessPoint_WPS_Object						*objAccessPoint_WPS[WL_MAX_BSSIDS];
	AccessPoint_AC_Object						*objAccessPoint_AC[WL_MAX_ACS];
	AccessPoint_Accounting_Object					*objAccessPoint_Accounting[WL_MAX_BSSIDS];
	AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object	*objAccessPoint_Security_X_COMCAST_COM_RadiusSettings[WL_MAX_BSSIDS];
}	wldm_cntxt_t;			/* WLDM Context */

static wldm_cntxt_t *wldm_context = NULL;
static int wldm_max_threads = WLDM_MAX_THREADS;

static wldm_cntxt_t *cntxt_get(void);
static int cntxt_free(void);

#define WLDM_CNTXT_FREE(err)		cntxt_free()

#define WLDM_CNTXT_INIT(err)		\
	wldm_cntxt_t *pCntx = cntxt_get(); \
	if (pCntx == NULL) {		\
		WIFI_DBG("%s: WLDM_CNTXT_INIT() failed!\n", __FUNCTION__);	\
		return err;		\
	};

#define WLDM_CNTXT_INIT_AND_LOCK(err)	\
	wldm_cntxt_t *pCntx;		\
	pthread_mutex_lock(&wldm_cntxt_mutex); \
	pCntx = cntxt_get();		\
	if (pCntx == NULL) {		\
		pthread_mutex_unlock(&wldm_cntxt_mutex); \
		WIFI_DBG("%s: WLDM_CNTXT_INIT_AND_LOCK() failed!\n", __FUNCTION__);	\
		return err;		\
	};

#define WLDM_CNTXT_GET(OBJ)		(pCntx->OBJ)

#define WLDM_CNTXT_GET_PTR(OBJ)		&(pCntx->OBJ)

#define WLDM_CNTXT_SET(OBJ, value)	(pCntx->OBJ) = (value)
#else
#define WLDM_CNTXT_FREE(err)
#define WLDM_CNTXT_INIT(err)
#define WLDM_CNTXT_INIT_AND_LOCK(err)	pthread_mutex_lock(&wldm_cntxt_mutex);
#define WLDM_CNTXT_GET(OBJ)		(OBJ)
#define WLDM_CNTXT_GET_PTR(OBJ)		&(OBJ)
#define WLDM_CNTXT_SET(OBJ, value)	(OBJ) = (value)

/* Defer actions mask of above ACTION bits */
static int actions_to_defer = 0;	/* Tell wldm_apply functions to defer the marked actions */
static int deferred_actions[MAX_WLAN_ADAPTER] = { 0 };
/* Let wldm_apply functions update their deferred actions, to be taken later at appropriate time */

/*
*  The configuration template objects.
*/
static Radio_Object							*objRadio[MAX_WLAN_ADAPTER] = { NULL };
static X_BROADCOM_COM_Radio_Object					*objX_BROADCOM_COM_Radio[MAX_WLAN_ADAPTER] = { NULL };
static X_RDK_Radio_Object						*objX_RDK_Radio[MAX_WLAN_ADAPTER] = { NULL };
static SSID_Object							*objSSID[WL_MAX_BSSIDS] = { NULL };
static X_LGI_Rates_Bitmap_Control_Object				*objX_LGI_RatesBitmap[WL_MAX_BSSIDS] = { NULL };
static AccessPoint_Object						*objAccessPoint[WL_MAX_BSSIDS] = { NULL };
static X_RDK_AccessPoint_Object						*objX_RDK_AccessPoint[WL_MAX_BSSIDS] = { NULL };
static X_BROADCOM_COM_AccessPoint_Object				*objX_BROADCOM_COM_AccessPoint[WL_MAX_BSSIDS] = { NULL };
static AccessPoint_Security_Object					*objAccessPoint_Security[WL_MAX_BSSIDS] = { NULL };
static X_RDK_AccessPoint_Security_Object				*objX_RDK_AccessPoint_Security[WL_MAX_BSSIDS] = { NULL };
static AccessPoint_WPS_Object						*objAccessPoint_WPS[WL_MAX_BSSIDS] = { NULL };
static AccessPoint_AC_Object						*objAccessPoint_AC[WL_MAX_ACS] = { NULL };
static AccessPoint_Accounting_Object					*objAccessPoint_Accounting[WL_MAX_BSSIDS] = { NULL };
static AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object	*objAccessPoint_Security_X_COMCAST_COM_RadiusSettings[WL_MAX_BSSIDS] = { NULL };
#endif /* WLDM_MAX_THREADS */

/* The dirver_lockfd[radioIndex] and nvram_lockfd are used by the main wldm_apply function so that
 * individual object apply functions do not need to get/free lock. If WLDM_TEST_LOCK is FALSE,
 * then the individual object appply function must get the necessary lock by itself.
 */
static int driver_lockfd[MAX_WLAN_ADAPTER] = { [0 ... (MAX_WLAN_ADAPTER - 1)] = -1 };
static int nvram_lockfd = -1;
#define WLDM_GET_LOCK(_lfd)		(_lfd)
#define WLDM_TEST_LOCK(idx)		(((driver_lockfd[idx] >= 0) || \
					  (nvram_lockfd >= 0)) ? TRUE : FALSE)

#define OBJ_MASK_CHECK(m, masks, ret)			\
do { \
	if (((m) & ~(masks)) || (((m) & (masks)) == 0)) { \
		WIFI_DBG("%s: invalid mask 0x%0x specified.\n", __FUNCTION__, (m)); \
		return (ret); \
	} \
}	while (0)

#define GET_OBJECT(pObj, aObj, mask)	\
do { \
	pObj = WLDM_CNTXT_GET(aObj); \
	if (pObj == NULL) { \
		pObj = calloc(1, sizeof(*pObj)); \
		if (pObj) WLDM_CNTXT_SET(aObj, pObj); \
	} else if (((pObj)->apply_map | (pObj)->reject_map) & (mask)) { \
		WIFI_DBG("%s: mask 0x%x is already set(0x%x/0x%x)!\n", \
			__FUNCTION__, (mask), (pObj)->apply_map, (pObj)->reject_map); \
	} \
	if (pObj == NULL) \
		pthread_mutex_unlock(&wldm_cntxt_mutex); \
}	while (0)

/* Topology info shall be dynamically initialized by wldm_init(). */
static int number_of_radios = DEFAULT_RUMTIME_RADIOS;
static int max_num_of_aps = DEFAULT_RUMTIME_RADIOS * WL_MAX_NUM_SSID;

static char const *radio_nvifname[] = {
	"wl0",
	"wl1",
	"wl2",
};	/* Indexed by radioIndex */

#if (MAX_WLAN_ADAPTER >= 3)
static char const *bss_x3_nvifname[] = {
	"wl0",		"wl1",
	"wl0.1",	"wl1.1",
	"wl0.2",	"wl1.2",
	"wl0.3",	"wl1.3",
	"wl0.4",	"wl1.4",
	"wl0.5",	"wl1.5",
	"wl0.6",	"wl1.6",
	"wl0.7",	"wl1.7",
	"wl2",		"wl2.1",
	"wl2.2",	"wl2.3",
	"wl2.4",	"wl2.5",
	"wl2.6",	"wl2.7",
};	/* Indexed by apIndex */
#endif /* MAX_WLAN_ADAPTER >= 3 */

#if (MAX_WLAN_ADAPTER >= 2)
static char const *bss_x2_nvifname[] = {
	"wl0",		"wl1",
	"wl0.1",	"wl1.1",
	"wl0.2",	"wl1.2",
	"wl0.3",	"wl1.3",
	"wl0.4",	"wl1.4",
	"wl0.5",	"wl1.5",
	"wl0.6",	"wl1.6",
	"wl0.7",	"wl1.7",
};	/* Indexed by apIndex */
#endif /* MAX_WLAN_ADAPTER >= 2 */

static char const *bss_x1_nvifname[] = {
	"wl0",
	"wl0.1",
	"wl0.2",
	"wl0.3",
	"wl0.4",
	"wl0.5",
	"wl0.6",
	"wl0.7",
};	/* Indexed by apIndex */

static char const **wl_ifname = DEFAULT_IFNAMES;
static char const **radio_ifname = DEFAULT_RADIO_IFNAMES;

static char *wl_osifname = NULL;
static char *radio_osifname = NULL;

static char *wldm_driver_lock[MAX_WLAN_ADAPTER] = {
	"/tmp/wl0_dm_drvlock",
#if (MAX_WLAN_ADAPTER >= 2)
	"/tmp/wl1_dm_drvlock",
#endif /* MAX_WLAN_ADAPTER >= 2 */
#if (MAX_WLAN_ADAPTER >= 3)
	"/tmp/wl2_dm_drvlock",
#endif /* MAX_WLAN_ADAPTER >= 3 */
};
static char *wldm_nvram_lock = "/tmp/wl_dm_nvramlock";

static char *wldm_escan_lock[MAX_WLAN_ADAPTER] = {
	"/tmp/wl0_escan_drvlock",
#if (MAX_WLAN_ADAPTER >= 2)
	"/tmp/wl1_escan_drvlock",
#endif /* MAX_WLAN_ADAPTER >= 2 */
#if (MAX_WLAN_ADAPTER >= 3)
	"/tmp/wl2_escan_drvlock",
#endif /* MAX_WLAN_ADAPTER >= 3 */
};

static char *wldm_wsec_lock[MAX_WLAN_ADAPTER] = {
	"/tmp/wl0_dm_wseclock",
#if (MAX_WLAN_ADAPTER >= 2)
	"/tmp/wl1_dm_wseclock",
#endif /* MAX_WLAN_ADAPTER >= 2 */
#if (MAX_WLAN_ADAPTER >= 3)
	"/tmp/wl2_dm_wseclock",
#endif /* MAX_WLAN_ADAPTER >= 3 */
};

/* The radio_deferred_actions[] are per process for all threads */
static pthread_mutex_t		radio_deferred_actions_mutex = PTHREAD_MUTEX_INITIALIZER;
static int			radio_deferred_actions[MAX_WLAN_ADAPTER] = { 0 };

static int do_apply_all(void);
static int do_apply(int radioIndex, int acts_to_defer);
static int schedule_deferred_actions(int radioIndex);
static int do_deferred_actions(void *arg);

#if defined(WLDM_APPLY_TIMER) || defined(WLDM_DEFER_TIMER)
/* Multiple timers/threads support by linux process timer.
 * When the timer expires, a thread is created to execute the given function.
 * Note that libshared implements its own timer_create/timer_delete functions.
 * Make sure -lrt is linked before -lshared.
 */

static void
sigev_thread_handler(union sigval sigv)
{
	timer_info *ptinfo = (timer_info *)sigv.sival_ptr;
	void *arg;
	int (*func)(void *arg);

	pthread_mutex_lock(&ptinfo->lock);
	if (ptinfo->repeat_count > 0)
		ptinfo->repeat_count--;
	arg = ptinfo->arg;
	func = ptinfo->func;
	if (ptinfo->repeat_count == 0) {
		struct itimerspec tspec = { {0}, };

		if (timer_settime(ptinfo->timerid, 0, &tspec, NULL) == 0)
			WIFI_DBG("%s: timerid %p stopped!\n", __FUNCTION__, ptinfo->timerid);
	}
	pthread_mutex_unlock(&ptinfo->lock);

	if (func) {
		int ret = func(arg);

		if (ret < 0) {
			WIFI_ERR("%s: callback ret %d!\n", __FUNCTION__, ret);
		}
	}
}

/* To create and arm a linux timer.
 * Upon timeout, the generic sigev_thread_handler() will be called,
 * which in term calls the given callback function and arg.
 *
 * ptinfo: the timer_info pointer.
 * ms: time in miliseconds.
 * count: number of times to repeat before stopping the timer.
 *    < 0, non-stop periodic timer.
 *    0, delete the linux timer.
 *    1, one shot.
 *    > 1, numbers to timeouts to repeat.
 * func, arg: the callback func and its argument.
 */
static int
arm_timer(timer_info *ptinfo, int ms, int count, void *func, void *arg)
{
	int ret;
	struct itimerspec tspec;

	pthread_mutex_lock(&ptinfo->lock);
	if (count == 0)	{
		if (ptinfo->timerid) {
			/* Delete the timer */
			timer_delete(ptinfo->timerid);
			ptinfo->timerid = NULL;
			ptinfo->arg = NULL;
			ptinfo->func = NULL;
		}
		pthread_mutex_unlock(&ptinfo->lock);
		return 0;
	}

	if (ptinfo->timerid == NULL) {
		struct sigevent sigev;

		memset(&sigev, 0, sizeof(sigev));
		sigev.sigev_notify = SIGEV_THREAD;
		sigev.sigev_notify_function = &sigev_thread_handler;
		sigev.sigev_value.sival_ptr = ptinfo;

		if (timer_create(CLOCK_REALTIME, &sigev, &ptinfo->timerid) < 0) {
			WIFI_ERR("%s: timer_create failed %d!\n", __FUNCTION__, errno);
			ptinfo->timerid = NULL;
			pthread_mutex_unlock(&ptinfo->lock);
			return -1;
		}
	}

	memset(&tspec, 0, sizeof(tspec));
	tspec.it_value.tv_sec = ms / 1000;
	tspec.it_value.tv_nsec = (ms % 1000) * 1000000;
	if (count != 1) {
		/* Periodic timer */
		tspec.it_interval = tspec.it_value;
	}

	/* Modify the timer info */
	if (ptinfo->repeat_count != count || ptinfo->arg != arg || ptinfo->func != func) {
		ptinfo->repeat_count = count;
		ptinfo->arg = arg;
		ptinfo->func = func;
	}
	ret = timer_settime(ptinfo->timerid, 0, &tspec, NULL);
	pthread_mutex_unlock(&ptinfo->lock);

	if (ret < 0) {
		WIFI_ERR("%s: timerid %p timer_settime failed %d!\n",
			__FUNCTION__, ptinfo->timerid, errno);
	} else {
		WIFI_DBG("%s: timerid %p count %d armed for %d ms!\n",
			__FUNCTION__, ptinfo->timerid, ptinfo->repeat_count, ms);
	}
	return ret;
}
#endif /* WLDM_APPLY_TIMER || WLDM_DEFER_TIMER */

#ifdef WLDM_DEFER_TIMER
static timer_info defer_timerinfo = { PTHREAD_MUTEX_INITIALIZER, 0 };
static int
defer_timer_start(int ms)
{
	if (arm_timer(&defer_timerinfo, ms, 1, &do_deferred_actions, NULL) < 0)
		return -1;

	return 0;
}
#endif /* WLDM_DEFER_TIMER */

#ifdef WLDM_APPLY_TIMER
/* Per process Apply timer. It times out after auto_apply_timeout of the last CMD_SET operation.
 * The auto_apply_timeout value can be controlled by nvram "wldm_auto_apply_timeout" in wldm_init.
 * Set it to 0 to disable auto apply feature.
 */
static int auto_apply_timeout = WLDM_AUTO_APPLY_TIME_MS;
#ifndef WLDM_MAX_THREADS
static timer_info apply_timerinfo = { .lock = PTHREAD_MUTEX_INITIALIZER };
#endif /* WLDM_MAX_THREADS */

/* The function called by the timeout-handling thread for auto apply timer */
static int
auto_apply_task(void *arg)
{
	int ret;
	pid_t tid = syscall(__NR_gettid);
#ifdef WLDM_MAX_THREADS
	wldm_cntxt_t *cntx = arg; /* The context of the thread which schedules the timer */
#endif /* WLDM_MAX_THREADS */

	pthread_mutex_lock(&wldm_cntxt_mutex);
#ifdef WLDM_MAX_THREADS
	/* Hijack the context of the timer-scheduling thread */
	cntx->tid = tid;
#endif /* WLDM_MAX_THREADS */

	WIFI_DBG("%s: tid %d calling do_apply_all()...\n", __FUNCTION__, tid);
	ret = do_apply_all();
	WIFI_DBG("%s: tid %d do_apply_all() ret %d done!\n", __FUNCTION__, tid, ret);
	pthread_mutex_unlock(&wldm_cntxt_mutex);

	return ret;
}

int
wldm_apply_timer_start(int ms)
{
	timer_info *ptimerinfo;
	int ret = 0;
#ifdef WLDM_MAX_THREADS
	WLDM_CNTXT_INIT(-1);
#else
	void *pCntx = NULL;
#endif /* WLDM_MAX_THREADS */

	ptimerinfo = WLDM_CNTXT_GET_PTR(apply_timerinfo);
	if (ms < 0) {
		/* Delete the timer */
		if (arm_timer(ptimerinfo, ms, 0, &auto_apply_task, pCntx) < 0)
			ret = -2;
	} else if (arm_timer(ptimerinfo, ms, 1, &auto_apply_task, pCntx) < 0) {
		/* If ms is 0, stop the timer, otherwise start as one-shot timer */
		ret = -1;
	}

	return ret;
}

#define START_APPLY_TIMER(ms)	\
do { \
	if ((ms) > 0 && wldm_apply_timer_start((ms)) < 0) { \
		WIFI_ERR("%s: apply timer start failed!", __FUNCTION__); \
	} \
}	while (0)
#define STOP_APPLY_TIMER()	\
do { \
	WIFI_DBG("%s: stop auto apply timer!\n", __FUNCTION__); \
	if (wldm_apply_timer_start(0) < 0) { \
		WIFI_ERR("%s: apply timer stop failed!", __FUNCTION__); \
	} \
}	while (0)
#define DEL_APPLY_TIMER()	\
do { \
	if (wldm_apply_timer_start(-1) < 0) { \
		WIFI_ERR("%s: apply timer delete failed!", __FUNCTION__); \
	} \
}	while (0)
#else
#define START_APPLY_TIMER(ms)
#define STOP_APPLY_TIMER()
#define DEL_APPLY_TIMER()
#endif /* WLDM_APPLY_TIMER */

#define DM_DRIVER_LOCK						1
#define NVRAM_LOCK						2
#define ESCAN_RESULTS_LOCK					3
#define WSEC_LOCK						4

static int
wldm_get_lock(int radioIndex, int lock)
{
	int lfd;
	pid_t tid = syscall(__NR_gettid);
	char *lfname;

	/* Aquire the temp file lock */
	if (lock == NVRAM_LOCK) {
		lfname = wldm_nvram_lock;
	} else if (lock == ESCAN_RESULTS_LOCK) {
		lfname = wldm_escan_lock[radioIndex];
	} else if (lock == WSEC_LOCK) {
		lfname = wldm_wsec_lock[radioIndex];
	} else {
		lfname = wldm_driver_lock[radioIndex];
	}
	lfd = open(lfname, O_RDONLY | O_CREAT | O_CLOEXEC, 0444);
	if (lfd < 0) {
		WIFI_ERR("%s: [%s(%d)] failed to open %s, %s!\n",
			__FUNCTION__, __progname, tid, lfname, strerror(errno));
		return -1;
	}
	if (flock(lfd, LOCK_EX) < 0) {
		WIFI_ERR("%s: [%s(%d)] failed to lock %s, %s!\n",
			__FUNCTION__, __progname, tid, lfname, strerror(errno));
		close(lfd);
		return -2;
	}

	WIFI_DBG("%s: [%s(%d)] locks %d->%s!\n", __FUNCTION__, __progname, tid, lfd, lfname);
	return lfd;
}

static int
wldm_free_lock(int lfd)
{
	pid_t tid = syscall(__NR_gettid);

	/* Release the temp file lock */
	WIFI_DBG("%s: [%s(%d)] unlocks %d!\n", __FUNCTION__, __progname, tid, lfd);
	flock(lfd, LOCK_UN);
	close(lfd);
	return 0;
}

static int
wldm_get_drvlock(int radioIndex)
{
	return wldm_get_lock(radioIndex, DM_DRIVER_LOCK);
}

static int
wldm_get_nvramlock(void)
{
	return wldm_get_lock(0, NVRAM_LOCK);
}

int
wldm_get_escanlock(int radioIndex)
{
	return wldm_get_lock(radioIndex, ESCAN_RESULTS_LOCK);
}

int
wldm_free_escanlock(int lfd)
{
	return wldm_free_lock(lfd);
}

int
wldm_get_wseclock(int radioIndex)
{
	return wldm_get_lock(radioIndex, WSEC_LOCK);
}

#ifdef WLDM_MAX_THREADS
static int
cntxt_init(void)
{
	int ret = 0;
	char *nvValStr = wlcsm_nvram_get("wldm_max_threads");

	pthread_mutex_lock(&wldm_cntxt_mutex);
	if (wldm_context) {
		pthread_mutex_unlock(&wldm_cntxt_mutex);
		return ret;
	}

	if (nvValStr && ((wldm_max_threads = atoi(nvValStr)) <= 1)) {
		WIFI_ERR("%s: invalid wldm_max_threads %d, use default %d!\n",
			__FUNCTION__, wldm_max_threads, WLDM_MAX_THREADS);
		wldm_max_threads = WLDM_MAX_THREADS;
	}
	wldm_context = (wldm_cntxt_t *)calloc(wldm_max_threads, sizeof(*wldm_context));
	if (wldm_context == NULL) {
		WIFI_ERR("%s: out of memory for thread contexts!\n", __FUNCTION__);
		ret = -1;
	}
	pthread_mutex_unlock(&wldm_cntxt_mutex);

	return ret;
}

static int
cntxt_free(void)
{
	int i;
	pid_t tid, pid;
	wldm_cntxt_t *pcontext;

	if (wldm_context == NULL) {
		WIFI_ERR("%s: wldm_context is not initialized for pid %d!", __FUNCTION__, getpid());
		return -1;
	}

	/* wldm_cntxt_mutex shall be locked already before calling this function */
	tid = syscall(__NR_gettid); /* gettid() */
	pid = getpid();
	for (i = 0; i < wldm_max_threads; i++) {
		if (wldm_context[i].tid == tid)
			break;
	}
	if (i >= wldm_max_threads) {
		return -1;
	}

	WIFI_DBG("%s: tid %d of pid %d at %d!\n", __FUNCTION__, tid, pid, i);

	pcontext = &wldm_context[i];
#if defined(WLDM_APPLY_TIMER)
	if (pcontext->apply_timerinfo.timerid)
		arm_timer(&pcontext->apply_timerinfo, 0, 0, NULL, NULL);
#endif /* WLDM_APPLY_TIMER */
	pcontext->tid = -1;
	return 0;
}

static wldm_cntxt_t *
cntxt_get(void)
{
	int i, free_index = -1;
	pid_t tid = syscall(__NR_gettid); /* gettid() */
	wldm_cntxt_t *pcontext;

	if (wldm_context == NULL) {
		WIFI_ERR("%s: wldm_context is not initialized for pid %d!", __FUNCTION__, getpid());
		return NULL;
	}

	/* wldm_cntxt_mutex shall be locked already before calling this function */
	for (i = 0; i < wldm_max_threads; i++) {
		if (wldm_context[i].tid == tid)
			break;
		if (free_index < 0 && wldm_context[i].tid <= 0)
			free_index = i;
	}
	if (i < wldm_max_threads) {
		pcontext = &wldm_context[i];
		WIFI_DBG("%s: tid %d of pid %d found at %d.\n", __FUNCTION__, pcontext->tid,
			getpid(), i);
		return pcontext;
	} else if (free_index < 0) {
		WIFI_ERR("%s: out of context for tid %d of pid %d!\n", __FUNCTION__, tid, getpid());
		return NULL;
	}

	/* New thread */
	pcontext = &wldm_context[free_index];
	memset(pcontext, 0, sizeof(*pcontext));
#if defined(WLDM_APPLY_TIMER)
	pthread_mutex_init(&pcontext->apply_timerinfo.lock, NULL);
#endif /* WLDM_APPLY_TIMER */
	pcontext->tid = tid;

	WIFI_DBG("%s: tid %d of pid %d allocated at %d.\n", __FUNCTION__, pcontext->tid,
		getpid(), free_index);
	return pcontext;
}
#endif /* WLDM_MAX_THREADS */

bool
hapd_disabled()
{
	static int hapd_disable_flag = -1;

	if (-1 == hapd_disable_flag) {
		char *enable = wlcsm_nvram_get("hapd_enable");

		hapd_disable_flag = (!enable || *enable == '0') ? 1 : 0;
	}

	return (1 == hapd_disable_flag) ? TRUE : FALSE;
}

/*
*  Specify the number of radios to manage at runtime. If radio <= 0, all detected radios
*  will be managed.
*  The compilation default is 2, call this function is necessary for 3 or 1 radio cases.
*/
int
wldm_init(int radios)
{
	int total, len = sizeof(total), i;
	char *nvValStr;

	if (wldm_RadioNumberOfEntries(CMD_GET, -1, (unsignedInt *)&total, &len, NULL, NULL) < 0) {
		WIFI_ERR("%s: wldm_RadioNumberOfEntries() failed!\n", __FUNCTION__);
		return -1;
	}
	if (radios <= 0)
		radios = total;

	if (radios > total || radios < 1) {
		WIFI_ERR("%s: total %d, cannot manage %d radios!\n", __FUNCTION__, total, radios);
		return -2;
	}

	if (number_of_radios == radios) {
		goto INIT_OS_IFNAME;
	}

	number_of_radios = radios;
	radio_ifname= DEFAULT_RADIO_IFNAMES;
	switch (number_of_radios) {
#if (MAX_WLAN_ADAPTER >= 3)
		case 3:
			wl_ifname = bss_x3_nvifname;
			break;
#endif /* MAX_WLAN_ADAPTER >= 3 */
#if (MAX_WLAN_ADAPTER >= 2)
		case 2:
			wl_ifname = bss_x2_nvifname;
			break;
#endif /* MAX_WLAN_ADAPTER >= 2 */
		default:
			wl_ifname = bss_x1_nvifname;
			break;
	}
	max_num_of_aps = number_of_radios * WL_MAX_NUM_SSID;

INIT_OS_IFNAME:
	if (wl_osifname)
		free(wl_osifname);
	wl_osifname = (char *)calloc(max_num_of_aps, IFNAMSIZ);
	if (!wl_osifname) {
		WIFI_ERR("%s: Fail to alloc memory for wl_osifname!\n", __FUNCTION__);
		return -1;
	}
	for (i = 0; i < max_num_of_aps; ++i) {
		if (nvifname_to_osifname(wl_ifname[i], wl_osifname + i * IFNAMSIZ, IFNAMSIZ) != 0) {
			WIFI_ERR("%s: Fail to convert nvifname to osifname!\n", __FUNCTION__);
			strncpy(wl_osifname + i * IFNAMSIZ, wl_ifname[i], IFNAMSIZ - 1);
		}
	}

	if (radio_osifname)
		free(radio_osifname);
	radio_osifname = (char *)calloc(number_of_radios, IFNAMSIZ);
	if (!radio_osifname) {
		WIFI_ERR("%s: Fail to alloc memory for radio_osifname!\n", __FUNCTION__);
		return -1;
	}
	for (i = 0; i < number_of_radios; ++i) {
		if (nvifname_to_osifname(radio_ifname[i], radio_osifname + i * IFNAMSIZ, IFNAMSIZ) != 0) {
			WIFI_ERR("%s: Fail to convert nvifname to osifname!\n", __FUNCTION__);
			strncpy(radio_osifname + i * IFNAMSIZ, radio_ifname[i], IFNAMSIZ - 1);
		}
	}

	/* use nvram set wldm_msglevel=0x0f for example as needed */
	nvValStr = wlcsm_nvram_get(NVRAM_WLDM_MSGLEVEL);
	if (nvValStr != NULL) {
		wldm_set_wldm_msglevel(strtoul(nvValStr, NULL, 0));
	} else {
		WIFI_DBG("[%s] %s wldm_msglevel = 0x%lx\n", __progname, __FUNCTION__, wldm_msglevel);
	}

#ifdef WLDM_MAX_THREADS
	if (cntxt_init() < 0) {
		if (wl_osifname)
			free(wl_osifname);
		if (radio_osifname)
			free(radio_osifname);
		return -1;
	}
#endif /* WLDM_MAX_THREADS */

#ifdef WLDM_APPLY_TIMER
	nvValStr = wlcsm_nvram_get("wldm_auto_apply_timeout");
	if (nvValStr != NULL) {
		auto_apply_timeout = atoi(nvValStr);
	}
#endif /* WLDM_APPLY_TIMER */
	return 0;
}

int
wldm_deinit(void)
{
	if (wl_osifname) {
		free(wl_osifname);
		wl_osifname = NULL;
	}

	if (radio_osifname) {
		free(radio_osifname);
		radio_osifname = NULL;
	}
	return 0;
}

int
wldm_set_wldm_msglevel(unsigned long msglevel)
{
	wldm_msglevel = msglevel;
	WIFI_DBG("[%s] %s wldm_msglevel = 0x%lx\n", __progname, __FUNCTION__, wldm_msglevel);
	return 0;
}

int
wldm_get_radios(void)
{
	return number_of_radios;
}

int
wldm_get_max_aps(void)
{
	return (max_num_of_aps);
}

char *
wldm_get_nvifname(int apIndex)
{
	APINDEX_CHECK(apIndex, NULL);
	return (char *)wl_ifname[apIndex];
}

char *
wldm_get_osifname(int apIndex)
{
	APINDEX_CHECK(apIndex, NULL);

	if (!wl_osifname && (wldm_init(-1) < 0)) {
		static char osifname[IFNAMSIZ];

		WIFI_DBG("%s: wldm_init failed!\n", __FUNCTION__);
		if (nvifname_to_osifname(wl_ifname[apIndex], osifname, sizeof(osifname)) != 0) {
			WIFI_DBG("%s: Fail to convert %s to osifname!\n", __FUNCTION__, wl_ifname[apIndex]);
			return (char *)wl_ifname[apIndex];
		} else {
			return osifname;
		}
	}
	return wl_osifname + apIndex * IFNAMSIZ;
}

char *
wldm_get_radio_nvifname(int radioIndex)
{
	RADIOINDEX_CHECK(radioIndex, NULL);
	return (char *)radio_ifname[radioIndex];
}

char *
wldm_get_radio_osifname(int radioIndex)
{
	RADIOINDEX_CHECK(radioIndex, NULL);

	if (!radio_osifname && (wldm_init(-1) < 0)) {
		static char radioOsifname[IFNAMSIZ];

		WIFI_DBG("%s: wldm_init failed!\n", __FUNCTION__);
		if (nvifname_to_osifname(radio_ifname[radioIndex], radioOsifname,
			sizeof(radioOsifname)) != 0) {
			WIFI_DBG("%s: Fail to convert %s to radioOsifname!\n", __FUNCTION__,
				radio_ifname[radioIndex]);
			return (char *)radio_ifname[radioIndex];
		} else {
			return radioOsifname;
		}
	}

	return radio_osifname + radioIndex * IFNAMSIZ;
}

int
wldm_get_radioIndex(int apIndex)
{
	APINDEX_CHECK(apIndex, -1);
	return RADIO_INDEX(apIndex);
}

int
wldm_get_bssidx(int apIndex)
{
	APINDEX_CHECK(apIndex, -1);
	return BSS_IDX(apIndex);
}

int
wldm_get_apindex(char *osifname)
{
	int i;
	char nvifname[IFNAMSIZ];

	osifname_to_nvifname(osifname, nvifname, sizeof(nvifname));

	for (i = 0; i < max_num_of_aps; i++) {
		if (strcmp(wl_ifname[i], nvifname) == 0) {
			return i;
		}
	}
	return -1;
}

#ifdef PHASE2_SEPARATE_RC
/* Start wireless security related daemons */
/* Use flock() to serialize multiple processes calling wldm_(re)start_wsec_daemons lib functions.
 * Since flock is per process, use _wsec_mutex to protect multiple threads in a process.
 */
static pthread_mutex_t _wsec_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif /* PHASE2_SEPARATE_RC */

#ifdef WLDM_APPLY_TIMER
static int
schedule_action(int radioIndex, int action_map, int ms)
{
	if (ms < 0)
		return -1;

	pthread_mutex_lock(&radio_deferred_actions_mutex);
	radio_deferred_actions[radioIndex] |= action_map;
	pthread_mutex_unlock(&radio_deferred_actions_mutex);

	if (wldm_apply_timer_start(ms) < 0)
		WIFI_ERR("%s: apply timer start failed!", __FUNCTION__);
	else
		WIFI_DBG("%s: radio %d map %x done!\n", __FUNCTION__, radioIndex, action_map);

	return 0;
}
#endif /* WLDM_APPLY_TIMER */

static int
stop_wsec_daemons(int radioIndex)
{
#ifdef PHASE2_SEPARATE_RC
	char buf[256];
	int ret, lfd;

	pthread_mutex_lock(&_wsec_mutex);
	lfd = wldm_get_wseclock(radioIndex);
	if (lfd < 0) {
		WIFI_DBG("%s: No lock is aquired for radio %d!\n", __FUNCTION__, radioIndex);
	}
	snprintf(buf, sizeof(buf), "wifi_setup.sh stop_security_daemons %s",
		wldm_get_radio_osifname(radioIndex));
	WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
	ret = system(buf);
	if (lfd >= 0) {
		/* Release the lock */
		wldm_free_lock(lfd);
	}
	pthread_mutex_unlock(&_wsec_mutex);
	if (ret != 0)
		WIFI_ERR("%s: Fail to run %s\n", __FUNCTION__, buf);

	return ret;
#else /* PHASE2_SEPARATE_RC */
	pid_t pid;

	if (!HAPD_DISABLED()) {
		char cmd[256];
		char *osifname;

		WIFI_DBG("%s on radio %d\n", __FUNCTION__, radioIndex);

		osifname = wldm_get_radio_osifname(radioIndex);
		snprintf(cmd, sizeof(cmd), "ifconfig %s down 2>/dev/null", osifname);
		WIFI_DBG("%s %s\n", __FUNCTION__, cmd);
		system(cmd);

		snprintf(cmd, sizeof(cmd), "wlconf %s down", osifname);
		WIFI_DBG("%s %s\n", __FUNCTION__, cmd);
		system(cmd);

		snprintf(cmd, sizeof(cmd), "hapd_conf %d stop", radioIndex);
		WIFI_DBG("%s %s\n", __FUNCTION__, cmd);
		system(cmd);
	} else {
		system("killall -q -15 wps_monitor");
		sleep(1);		/* Delay to allow wps_monitor to tear down */
		system("killall -q -9 nas");
		WIFI_DBG("%s: nas/wps/bsd killed!\n", __FUNCTION__);
	}

	pid = get_pid_by_name("eapd");
	if (pid > 0) {
		dm_unregister_app_restart_info(pid);
		system("killall -q -9 eapd 2>/dev/null");
		WIFI_DBG("%s: eapd killed!\n", __FUNCTION__);
	}
	else
		WIFI_ERR("%s: eapd is not running!\n", __FUNCTION__);

	pid = get_pid_by_name("bsd");
	if (pid > 0) {
		dm_unregister_app_restart_info(pid);
		system("killall -q -15 bsd");
		WIFI_DBG("%s: bsd killed!\n", __FUNCTION__);
	}
	else
		WIFI_ERR("%s: bsd is not running!\n", __FUNCTION__);

	return 0;
#endif /* PHASE2_SEPARATE_RC */
}

static int
restart_wsec_daemons(int radioIndex)
{
#ifdef PHASE2_SEPARATE_RC
	char buf[256], *osifname = wldm_get_radio_osifname(radioIndex);
	int ret, lfd;

	pthread_mutex_lock(&_wsec_mutex);
	lfd = wldm_get_wseclock(radioIndex);
	if (lfd < 0) {
		WIFI_DBG("%s: No lock is aquired for radio %d!\n", __FUNCTION__, radioIndex);
	}
	snprintf(buf, sizeof(buf), "wifi_setup.sh stop_security_daemons %s", osifname);
	WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
	ret = system(buf);
	snprintf(buf, sizeof(buf), "wifi_setup.sh start_security_daemons %s", osifname);
	WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
	ret = system(buf);
	if (lfd >= 0) {
		/* Release the lock */
		wldm_free_lock(lfd);
	}
	pthread_mutex_unlock(&_wsec_mutex);
	if (ret != 0)
		WIFI_ERR("%s: Fail to run %s\n", __FUNCTION__, buf);

	return ret;
#else /* PHASE2_SEPARATE_RC */
	if (!HAPD_DISABLED()) {
		char cmd[256];
		char *osifname;

		WIFI_DBG("%s on radio %d\n", __FUNCTION__, radioIndex);

		osifname = wldm_get_radio_osifname(radioIndex);
		snprintf(cmd, sizeof(cmd), "wlconf %s up", osifname);
		WIFI_DBG("%s %s\n", __FUNCTION__, cmd);
		system(cmd);

		snprintf(cmd, sizeof(cmd), "ifconfig %s up 2>/dev/null", osifname);
		WIFI_DBG("%s %s\n", __FUNCTION__, cmd);
		system(cmd);

		snprintf(cmd, sizeof(cmd), "hapd_conf %d start", radioIndex);
		WIFI_DBG("%s %s\n", __FUNCTION__, cmd);
		system(cmd);

		snprintf(cmd, sizeof(cmd), "wlconf %s start", osifname);
		WIFI_DBG("%s %s\n", __FUNCTION__, cmd);
		system(cmd);

		/* Reload wps_pbcd in case wps_mode has been changed */
		WIFI_DBG("%s reload wps_pbcd\n", __FUNCTION__);
		system("killall -SIGUSR1 wps_pbcd");
	} else {
		system("nas");
		system("rm -rf /tmp/wps_monitor.pid");
		system("wps_monitor&");
		WIFI_DBG("%s: nas/wps/bsd started!\n", __FUNCTION__);
	}
	system("eapd");
	system("bsd");		/* The bsd checks if the radios have the same credential. */
	return 0;
#endif /* PHASE2_SEPARATE_RC */
}

int
wldm_stop_wsec_daemons(int radioIndex)
{
#if defined(WLDM_APPLY_TIMER)
	schedule_action(radioIndex, ACTION_APP_HOSTAPD_STOP, 500);
	return 0;
#else
	return stop_wsec_daemons(radioIndex);
#endif /* WLDM_APPLY_TIMER */
}

int
wldm_start_wsec_daemons(int radioIndex)
{
#if defined(WLDM_APPLY_TIMER)
	schedule_action(radioIndex, ACTION_APP_HOSTAPD, 500);
	/* XXX sleep(8) is added in wifi_startHostApd in wifi_hal.c
	 * to WAR the GUI return in less than 1 sec issue.
	 */
	return 0;
#else
	return restart_wsec_daemons(radioIndex);
#endif /* WLDM_APPLY_TIMER */
}

int
wldm_restart_wsec_daemons(int radioIndex)
{
#if defined(WLDM_APPLY_TIMER)
	schedule_action(radioIndex, ACTION_APP_HOSTAPD_STOP | ACTION_APP_HOSTAPD, 1000);
	return 0;
#else
	return restart_wsec_daemons(radioIndex);
#endif /* WLDM_APPLY_TIMER */
}

/*
*  Object member functions.
*/
/* Radio object */

/* Return 0 if successful, < 0 error code */
int
wldm_free_RadioObject(int radioIndex)
{
	int index = radioIndex;
	Radio_Object *pObj;
	X_BROADCOM_COM_Radio_Object *pObjXBrcm;
	X_RDK_Radio_Object *pObjXRDK;
	WLDM_CNTXT_INIT(-1);

	RADIOINDEX_CHECK(index, -1);

	pObj = WLDM_CNTXT_GET(objRadio[index]);
	pObjXBrcm = WLDM_CNTXT_GET(objX_BROADCOM_COM_Radio[index]);
	pObjXRDK = WLDM_CNTXT_GET(objX_RDK_Radio[index]);

	WLDM_CNTXT_SET(objRadio[index], NULL);
	WLDM_CNTXT_SET(objX_BROADCOM_COM_Radio[index], NULL);
	WLDM_CNTXT_SET(objX_RDK_Radio[index], NULL);

	if (pObj != NULL) {
		/* Free the dynamically allocated RW contents(hexBinary, IPAddress, list, string) */
		if (pObj->Radio.Alias) {
			free(pObj->Radio.Alias);
			pObj->Radio.Alias = NULL;
		}
		if (pObj->Radio.LowerLayers) {
			free(pObj->Radio.LowerLayers);
			pObj->Radio.LowerLayers = NULL;
		}
		if (pObj->Radio.OperatingFrequencyBand) {
			free(pObj->Radio.OperatingFrequencyBand);
			pObj->Radio.OperatingFrequencyBand = NULL;
		}
		if (pObj->Radio.OperatingStandards) {
			free(pObj->Radio.OperatingStandards);
			pObj->Radio.OperatingStandards = NULL;
		}
		if (pObj->Radio.OperatingChannelBandwidth) {
			free(pObj->Radio.OperatingChannelBandwidth);
			pObj->Radio.OperatingChannelBandwidth = NULL;
		}
		if (pObj->Radio.ExtensionChannel) {
			free(pObj->Radio.ExtensionChannel);
			pObj->Radio.ExtensionChannel = NULL;
		}
		if (pObj->Radio.GuardInterval) {
			free(pObj->Radio.GuardInterval);
			pObj->Radio.GuardInterval = NULL;
		}
		if (pObj->Radio.RegulatoryDomain) {
			free(pObj->Radio.RegulatoryDomain);
			pObj->Radio.RegulatoryDomain = NULL;
		}
		if (pObj->Radio.CCARequest) {
			free(pObj->Radio.CCARequest);
			pObj->Radio.CCARequest = NULL;
		}
		if (pObj->Radio.RPIHistogramRequest) {
			free(pObj->Radio.RPIHistogramRequest);
			pObj->Radio.RPIHistogramRequest = NULL;
		}
		if (pObj->Radio.PreambleType) {
			free(pObj->Radio.PreambleType);
			pObj->Radio.PreambleType = NULL;
		}
		if (pObj->Radio.OperationalDataTransmitRates) {
			free(pObj->Radio.OperationalDataTransmitRates);
			pObj->Radio.OperationalDataTransmitRates = NULL;
		}
		if (pObj->Radio.BasicDataTransmitRates) {
			free(pObj->Radio.BasicDataTransmitRates);
			pObj->Radio.BasicDataTransmitRates = NULL;
		}

		free(pObj);
	}

	if (pObjXBrcm != NULL) {
		free(pObjXBrcm);
	}

	if (pObjXRDK != NULL) {
		free(pObjXRDK);
	}

	return 0;
}

void
wldm_rel_Object(void *pObj, bool start_auto_apply_timer)
{
#ifdef WLDM_APPLY_TIMER
	if (start_auto_apply_timer) {
		WIFI_DBG("%s: start_auto_apply_timer %d!\n", __FUNCTION__, start_auto_apply_timer);
		START_APPLY_TIMER(auto_apply_timeout);
	}
#endif /* WLDM_APPLY_TIMER */

	if (pObj) {
		/* Free the mutex locked in WLDM_CNTXT_INIT_AND_LOCK */
		pthread_mutex_unlock(&wldm_cntxt_mutex);
	}
}

/*
*  Return the Radio_Object pointer if previously allocated, otherwise alloc a new one.
*  radioIndex: Radio Index, starting from 0.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
Radio_Object *
wldm_get_RadioObject(int radioIndex, int checkMask)
{
	int index = radioIndex;
	Radio_Object *pObj;

	RADIOINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, Radio_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objRadio[index], checkMask);

	return pObj;
}

/*
*  Return the X_Broadcom_Radio_Object pointer if previously allocated, otherwise alloc a new one.
*  radioIndex: Radio Index, starting from 0.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
X_BROADCOM_COM_Radio_Object *
wldm_get_X_BROADCOM_COM_RadioObject(int radioIndex, int checkMask)
{
	int index = radioIndex;
	X_BROADCOM_COM_Radio_Object *pObj;

	RADIOINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, X_BROADCOM_COM_Radio_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objX_BROADCOM_COM_Radio[index], checkMask);

	return pObj;
}

/*
*  Return the X_RDK_Radio_Object pointer if previously allocated, otherwise alloc a new one.
*  radioIndex: Radio Index, starting from 0.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
X_RDK_Radio_Object *
wldm_get_X_RDK_RadioObject(int radioIndex, int checkMask)
{
	int index = radioIndex;
	X_RDK_Radio_Object *pObj;

	RADIOINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, X_RDK_Radio_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objX_RDK_Radio[index], checkMask);

	return pObj;
}

static int
wl_get_chan_bw_extChan(int radioIndex, Radio_Object *pObj, unsigned int *channel, int *bw, int *extChan)
{
	int chanspec, chanFromChanspec, chbw;
	char *nvifname, *chanspec_str, nvram_name[NVRAM_NAME_SIZE];

	nvifname = wldm_get_radio_nvifname(radioIndex);
	snprintf(nvram_name, sizeof(nvram_name), "%s_chanspec", nvifname);
	chanspec_str = wlcsm_nvram_get(nvram_name);
	if (chanspec_str == NULL) {
		WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
		return -1;
	}
	chanspec = wf_chspec_aton(chanspec_str);
	chanFromChanspec = wf_chspec_ctlchan(chanspec);

	if (pObj->apply_map & Radio_OperatingChannelBandwidth_MASK) {
		*bw = (strcmp(pObj->Radio.OperatingChannelBandwidth, "20MHz") == 0) ? 20 :
			(strcmp(pObj->Radio.OperatingChannelBandwidth, "40MHz") == 0) ? 40 :
			(strcmp(pObj->Radio.OperatingChannelBandwidth, "80MHz") == 0) ? 80 :
			(strcmp(pObj->Radio.OperatingChannelBandwidth, "160MHz") == 0) ? 160 :
			(strcmp(pObj->Radio.OperatingChannelBandwidth, "80+80") == 0) ? 8080 : 0;
	} else {
		chbw = (chanspec & WL_CHANSPEC_BW_MASK);
		*bw = (chbw  == WL_CHANSPEC_BW_20) ? 20 :
			(chbw  == WL_CHANSPEC_BW_40) ? 40 :
			(chbw  == WL_CHANSPEC_BW_80) ? 80 :
			(chbw  == WL_CHANSPEC_BW_160) ? 160 :
			(chbw  == WL_CHANSPEC_BW_8080) ? 8080 : 0;
	}

	if (pObj->apply_map & Radio_ExtensionChannel_MASK) {
		*extChan = (strcmp(pObj->Radio.ExtensionChannel, "AboveControlChannel") == 0) ? 1 :
			(strcmp(pObj->Radio.ExtensionChannel, "BelowControlChannel") == 0) ? -1 : 0;
	} else {
		*extChan = 0;
	}

	if (pObj->apply_map & Radio_Channel_MASK) {
		*channel = pObj->Radio.Channel;
		if (!(pObj->apply_map & Radio_ExtensionChannel_MASK) &&
			!(pObj->apply_map & Radio_OperatingChannelBandwidth_MASK)) {
			*bw = 0;
			*extChan = 0;
		}
	} else {
		*channel = chanFromChanspec;
	}

	return 0;
}

/* Return 0 if successful, < 0 error code */
static int
apply_RadioObject(int radioIndex)
{
	int index = radioIndex;
	int len, ret = 0, bw = 0, extChan = 0;
	unsigned int channel = 0;
	bool radio_isup, enable;
	int action = 0, acts_to_defer, deferred_acts = 0;
	int ioctl_map = 0, ioctl_map_XBrcm = 0, ioctl_wl_down = 0;	/* The map to configure driver by issuing ioctls. */
	int ioctl_map_XRDK = 0;
	int lfd = -1;
	Radio_Object *pObj;
	X_BROADCOM_COM_Radio_Object *pObjXBrcm;
	X_RDK_Radio_Object *pObjXRDK;
	char buf[256];
	unsigned int initSecs;
	pid_t pid;
	char *osifname;
	WLDM_CNTXT_INIT(-1);

	WIFI_INFO("%s Index=%d Enter\n", __FUNCTION__, radioIndex);

	osifname = wldm_get_radio_osifname(radioIndex);
	pObj = WLDM_CNTXT_GET(objRadio[index]);
	pObjXBrcm = WLDM_CNTXT_GET(objX_BROADCOM_COM_Radio[index]);
	pObjXRDK = WLDM_CNTXT_GET(objX_RDK_Radio[index]);
	if ((pObj == NULL) && (pObjXBrcm == NULL) && (pObjXRDK == NULL)) {
		return 0;
	}

	if (pObjXRDK && pObjXRDK->reject_map) {
		WIFI_DBG("Invalid parameters are present in X_RDK_Radio Object, reject map 0x%0x!\n",
			pObjXRDK->reject_map);
	}
	if (pObjXBrcm && pObjXBrcm->reject_map) {
		WIFI_DBG("Invalid parameters are present in X_BROADCOM_COM_Radio Object, reject map 0x%0x!\n",
			pObjXBrcm->reject_map);
	}

	if (pObj && pObj->reject_map) {
		WIFI_ERR("Invalid parameters are present in Radio Object, reject map 0x%0x!\n",
			pObj->reject_map);
		wldm_free_RadioObject(index);
		return -2;
	}

	if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
		/* No lock is done, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	if (pObj) {
		/* As of TR181 2.12, there are 27 RW parameters.
		*  Set the corresponding nvram variables, and determine how to restart.
		*/
		WIFI_DBG("%s: applying mapped 0x%0x params to %s: Device.WiFi.Radio.%d.\n",
			__FUNCTION__, pObj->apply_map, wl_ifname[index], index + 1);

		/* Consistency of OperatingFrequencyBand, OperatingStandards, Channel, etc.
		*  shall have already been done in wldm_check_apply().
		*/
		if (pObj->apply_map & Radio_OperatingFrequencyBand_MASK) {
			WIFI_DBG("\tOperatingFrequencyBand=%s\n",
					pObj->Radio.OperatingFrequencyBand);
		}
		if (pObj->apply_map & Radio_OperatingStandards_MASK) {
			len = strlen(pObj->Radio.OperatingStandards) + 1;
			WIFI_DBG("\tOperatingStandards=%s len=%d\n",
					pObj->Radio.OperatingStandards, len);
			if (wldm_Radio_OperatingStandards(CMD_SET_NVRAM, index,
				pObj->Radio.OperatingStandards, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_OperatingStandards_MASK;
			}
		}

		/* Auto Channel operation */
		if (pObj->apply_map & Radio_AutoChannelEnable_MASK) {
			WIFI_DBG("\tAutoChannelEnable=%d\n", (int)pObj->Radio.AutoChannelEnable);
			len = sizeof(pObj->Radio.AutoChannelEnable);
			if (wldm_Radio_AutoChannelEnable(CMD_SET_NVRAM, index,
				&(pObj->Radio.AutoChannelEnable), &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_AutoChannelEnable_MASK;
			}
		}

		if (pObj->apply_map & Radio_AutoChannelRefreshPeriod_MASK) {
			WIFI_DBG("\tAutoChannelRefreshPeriod=%d\n", pObj->Radio.AutoChannelRefreshPeriod);
			len = sizeof(pObj->Radio.AutoChannelRefreshPeriod);
			if (wldm_Radio_AutoChannelRefreshPeriod(CMD_SET_NVRAM, index,
				&pObj->Radio.AutoChannelRefreshPeriod, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_AutoChannelRefreshPeriod_MASK;
			}
		}

		wl_get_chan_bw_extChan(radioIndex, pObj, &channel, &bw, &extChan);
		if (pObj->apply_map & (Radio_OperatingChannelBandwidth_MASK |
			Radio_ExtensionChannel_MASK | Radio_Channel_MASK)) {
			if (pObj->apply_map & Radio_OperatingChannelBandwidth_MASK) {
				WIFI_DBG("\tOperatingChannelBandwidth=%s\n",
					pObj->Radio.OperatingChannelBandwidth);
				len = strlen(pObj->Radio.OperatingChannelBandwidth);
				if (wldm_Radio_OperatingChannelBandwidth(CMD_SET_NVRAM, index,
					pObj->Radio.OperatingChannelBandwidth, &len, NULL, NULL) == 0) {
					action |= ACTION_APP_NVRAM_COMMIT;
					ioctl_map |= Radio_OperatingChannelBandwidth_MASK;
					ioctl_wl_down = 1;
				}
			}
			if (pObj->apply_map & Radio_ExtensionChannel_MASK) {
				WIFI_DBG("\tExtensionChannel=%s\n", pObj->Radio.ExtensionChannel);
			}
			if (pObj->apply_map & Radio_Channel_MASK) {
				WIFI_DBG("\tChannel=%d\n", pObj->Radio.Channel);
			}

			if (channel == 0) {
				WIFI_DBG("%s: Set to run autochannel cmd\n", __FUNCTION__);
				pObj->Radio.AutoChannelEnable = TRUE;
				ioctl_map |= Radio_AutoChannelEnable_MASK;
			} else {
				len = sizeof(channel);
				if (wldm_Radio_Channel(CMD_SET_NVRAM, index, &channel,
					&len, bw, extChan, NULL, NULL) == 0) {
					action |= ACTION_APP_NVRAM_COMMIT;
					ioctl_map |= pObj->apply_map & (Radio_Channel_MASK |
						Radio_OperatingChannelBandwidth_MASK |
						Radio_ExtensionChannel_MASK);
					ioctl_wl_down = 1;
				}
			}
		}

		if (pObj->apply_map & Radio_Alias_MASK) {
			WIFI_DBG("\tAlias=%s\n", pObj->Radio.Alias);
			len = sizeof(pObj->Radio.Alias);
			if (wldm_Radio_Alias(CMD_SET_NVRAM, index,
				pObj->Radio.Alias, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & Radio_LowerLayers_MASK) {
			WIFI_DBG("\tLowerLayers=%s, ignored\n", pObj->Radio.LowerLayers);
			/* TR181: Radio.{i}.LowerLayers is not used. */
		}

		/* Rate related */
		if (pObj->apply_map & Radio_MCS_MASK) {
			WIFI_DBG("\tMCS=%d\n", pObj->Radio.MCS);
		}

		if (pObj->apply_map & Radio_BasicDataTransmitRates_MASK) {
			WIFI_DBG("\tBasicDataTransmitRates=%s\n", pObj->Radio.BasicDataTransmitRates);
			len = strlen(pObj->Radio.BasicDataTransmitRates) + 1;
			if (wldm_Radio_BasicDataTransmitRates(CMD_SET_NVRAM, index,
				pObj->Radio.BasicDataTransmitRates, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_BasicDataTransmitRates_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObj->apply_map & Radio_OperationalDataTransmitRates_MASK) {
			WIFI_DBG("\tOperationalDataTransmitRates=%s\n",
				pObj->Radio.OperationalDataTransmitRates);
		}

		/* Regulatory related */
		if (pObj->apply_map & Radio_RegulatoryDomain_MASK) {
			WIFI_DBG("\tRegulatoryDomain=%s\n", pObj->Radio.RegulatoryDomain);
			len = strlen(pObj->Radio.RegulatoryDomain);
			if (wldm_Radio_RegulatoryDomain(CMD_SET_NVRAM, index,
				pObj->Radio.RegulatoryDomain, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_RegulatoryDomain_MASK;
				ioctl_wl_down = 1;
			}
		}
		if (pObj->apply_map & Radio_TransmitPower_MASK) {
			WIFI_DBG("\tTransmitPower=%d\n", pObj->Radio.TransmitPower);
			/* Set nvram and issue IOCTL command later if necessary */
			ioctl_map |= Radio_TransmitPower_MASK;
		}
		if (pObj->apply_map & Radio_IEEE80211hEnabled_MASK) {
			WIFI_DBG("\tIEEE80211hEnabled=%d\n", (int)pObj->Radio.IEEE80211hEnabled);
			/* Set nvram and issue IOCTL command later if necessary */
			len = sizeof(pObj->Radio.IEEE80211hEnabled);
			if (wldm_Radio_IEEE80211hEnabled(CMD_SET_NVRAM, index,
				&pObj->Radio.IEEE80211hEnabled, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_IEEE80211hEnabled_MASK;
				ioctl_wl_down = 1;
			}
		}

		/* Measurement requests */
		if (pObj->apply_map & Radio_CCARequest_MASK) {
			WIFI_DBG("\tCCARequest=%s\n", pObj->Radio.CCARequest);
		}
		if (pObj->apply_map & Radio_RPIHistogramRequest_MASK) {
			WIFI_DBG("\tRPIHistogramRequest=%s\n", pObj->Radio.RPIHistogramRequest);
		}

		if (pObj->apply_map & Radio_Enable_MASK) {
			WIFI_DBG("\tEnable=%d\n", pObj->Radio.Enable);
			len = sizeof(pObj->Radio.Enable);
			if (wldm_Radio_Enable(CMD_SET_NVRAM, index,
				&pObj->Radio.Enable, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
				ioctl_map |= Radio_Enable_MASK;
			}
		}
		if (pObj->apply_map & Radio_FragmentationThreshold_MASK) {
			WIFI_DBG("\tFragmentationThreshold=%d\n", pObj->Radio.FragmentationThreshold);
			len = sizeof(pObj->Radio.FragmentationThreshold);
			if (wldm_Radio_FragmentationThreshold(CMD_SET_NVRAM, index,
				&pObj->Radio.FragmentationThreshold, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_FragmentationThreshold_MASK;
			}
		}
		if (pObj->apply_map & Radio_RTSThreshold_MASK) {
			WIFI_DBG("\tRTSThreshold=%d\n", pObj->Radio.RTSThreshold);
			len = sizeof(pObj->Radio.RTSThreshold);
			if (wldm_Radio_RTSThreshold(CMD_SET_NVRAM, index,
				&pObj->Radio.RTSThreshold, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_RTSThreshold_MASK;
			}
		}

		/* Retry limits */
		if (pObj->apply_map & Radio_RetryLimit_MASK) {
			WIFI_DBG("\tRetryLimit=%d\n", pObj->Radio.RetryLimit);
			ioctl_map |= Radio_RetryLimit_MASK;
		}
		if (pObj->apply_map & Radio_LongRetryLimit_MASK) {
			WIFI_DBG("\tLongRetryLimit=%d\n", pObj->Radio.LongRetryLimit);
			ioctl_map |= Radio_LongRetryLimit_MASK;
		}

		/* Beacons */
		if (pObj->apply_map & Radio_BeaconPeriod_MASK) {
			WIFI_DBG("\tBeaconPeriod=%d\n", pObj->Radio.BeaconPeriod);
			len = sizeof(pObj->Radio.BeaconPeriod);
			if (wldm_Radio_BeaconPeriod(CMD_SET_NVRAM, index,
				(int *) &pObj->Radio.BeaconPeriod, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
				ioctl_map |= Radio_BeaconPeriod_MASK;
			}
		}
		if (pObj->apply_map & Radio_DTIMPeriod_MASK) {
			WIFI_DBG("\tDTIMPeriod=%d\n", pObj->Radio.DTIMPeriod);
			len = sizeof(pObj->Radio.DTIMPeriod);
			if (wldm_Radio_DTIMPeriod(CMD_SET_NVRAM, index,
				(int *) &pObj->Radio.DTIMPeriod, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
				ioctl_map |= Radio_DTIMPeriod_MASK;
			}
		}

		/* PHY related */
		if (pObj->apply_map & Radio_PreambleType_MASK) {
			WIFI_DBG("\tPreambleType=%s\n", pObj->Radio.PreambleType);
		}
		if (pObj->apply_map & Radio_GuardInterval_MASK) {
			WIFI_DBG("\tGuardInterval=%s\n", pObj->Radio.GuardInterval);
			len = sizeof(pObj->Radio.GuardInterval);
			if (wldm_Radio_GuardInterval(CMD_SET_NVRAM, index,
				pObj->Radio.GuardInterval, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= Radio_GuardInterval_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObj->apply_map & Radio_PacketAggregationEnable_MASK) {
			WIFI_DBG("\tPacketAggregationEnable=%d\n",
				(int)pObj->Radio.PacketAggregationEnable);
		}

	}

	if (pObjXBrcm) {
		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_AxEnable_MASK) {
			WIFI_DBG("\tAxEnable=%d\n", (int) pObjXBrcm->X_BROADCOM_COM_Radio.AxEnable);
			ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_AxEnable_MASK;
			ioctl_wl_down = 1;
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_AxFeatures_MASK) {
			WIFI_DBG("\tAxFeatures=%d\n", pObjXBrcm->X_BROADCOM_COM_Radio.AxFeatures);
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.AxFeatures);
			if (wldm_AXfeatures(CMD_SET_NVRAM, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.AxFeatures), &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_AxFeatures_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_AxBsscolor_MASK) {
			WIFI_DBG("\tAxBsscolor=%d\n", pObjXBrcm->X_BROADCOM_COM_Radio.AxBsscolor);
			ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_AxBsscolor_MASK;
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_AxMuType_MASK) {
			WIFI_DBG("\tAxMuType=%d\n", pObjXBrcm->X_BROADCOM_COM_Radio.AxMuType);
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.AxMuType);
			if (wldm_xbrcm_Radio_AXmuType(CMD_SET_NVRAM, index,
				(he_mu_type_t *)&(pObjXBrcm->X_BROADCOM_COM_Radio.AxMuType),
				(uint *)&len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_AxMuType_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_AxMuEdca_MASK) {
			WIFI_DBG("\tAxMuEdcs aci=%d, aifsn=%d, ecw_min=%d, ecw_max=%d, timer=%d\n",
				pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca.aci,
				pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca.aifsn,
				pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca.ecw_min,
				pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca.ecw_max,
				pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca.timer);
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca);
			if (wldm_xbrcm_Radio_AXmuEdca(CMD_SET_NVRAM, index,
				&pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca,
				(uint *)&len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_AxMuEdca_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_CtsProtectionEnable_MASK) {
			WIFI_DBG("\tCtsProtectionEnable=%d\n", pObjXBrcm->X_BROADCOM_COM_Radio.CtsProtectionEnable);
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.CtsProtectionEnable);
			if (wldm_xbrcm_Radio_CtsProtectionEnable(CMD_SET_NVRAM, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.CtsProtectionEnable), &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_CtsProtectionEnable_MASK;
			}
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_STBCEnable_MASK) {
			WIFI_DBG("\tSTBCEnable=%d\n", pObjXBrcm->X_BROADCOM_COM_Radio.STBCEnable);
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.STBCEnable);
			if (wldm_xbrcm_Radio_STBCEnable(CMD_SET_NVRAM, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.STBCEnable), &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_STBCEnable_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_TxChainMask_MASK) {
			WIFI_DBG("\tTxChainMask=%d\n", pObjXBrcm->X_BROADCOM_COM_Radio.TxChainMask);
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.TxChainMask);
			if (wldm_xbrcm_Radio_TxChainMask(CMD_SET_NVRAM, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.TxChainMask), &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_TxChainMask_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_RxChainMask_MASK) {
			WIFI_DBG("\tRxChainMask=%d\n", pObjXBrcm->X_BROADCOM_COM_Radio.RxChainMask);
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.RxChainMask);
			if (wldm_xbrcm_Radio_RxChainMask(CMD_SET_NVRAM, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.RxChainMask), &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_RxChainMask_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXBrcm->apply_map & X_BROADCOM_COM_Radio_Greenfield11nEnable_MASK) {
			WIFI_DBG("\tGreenfield11nEnable=%d\n", pObjXBrcm->X_BROADCOM_COM_Radio.Greenfield11nEnable);
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.Greenfield11nEnable);
			if (wldm_xbrcm_Radio_Greenfield11nEnable(CMD_SET_NVRAM, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.Greenfield11nEnable), &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XBrcm |= X_BROADCOM_COM_Radio_Greenfield11nEnable_MASK;
				ioctl_wl_down = 1;
			}
		}

	}

	if (pObjXRDK) {
		if (pObjXRDK->apply_map & X_RDK_Radio_AmsduEnable_MASK) {
			WIFI_DBG("\tamsdu=%d\n", (int) pObjXRDK->X_RDK_Radio.amsdu);
			ioctl_map_XRDK |= X_RDK_Radio_AmsduEnable_MASK;
			ioctl_wl_down = 1;
		}

		if (pObjXRDK->apply_map & X_RDK_Radio_AutoChannelDwellTime_MASK) {
			WIFI_DBG("\tAutoChannelDwellTime=%d\n", pObjXRDK->X_RDK_Radio.AutoChannelDwellTime);
			len = sizeof(pObjXRDK->X_RDK_Radio.AutoChannelDwellTime);
			if (wldm_Radio_AutoChannelDwellTime(CMD_SET_NVRAM, index,
				&pObjXRDK->X_RDK_Radio.AutoChannelDwellTime, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XRDK |= X_RDK_Radio_AutoChannelDwellTime_MASK;
			}
		}

		if (pObjXRDK->apply_map & X_RDK_Radio_DfsEnable_MASK) {
			WIFI_DBG("\tDfsEnable=%d\n", pObjXRDK->X_RDK_Radio.DfsEnable);
			len = sizeof(pObjXRDK->X_RDK_Radio.DfsEnable);
			if (wldm_Radio_DfsEnable(CMD_SET_NVRAM, index,
				&pObjXRDK->X_RDK_Radio.DfsEnable, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XRDK |= X_RDK_Radio_DfsEnable_MASK;
				ioctl_wl_down = 1;
			}
		}
	}
	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	if (action & ACTION_SYS_RESTART) {
#ifdef PHASE2_SEPARATE_RC
		snprintf(buf, sizeof(buf), "wifi_setup.sh restart %s", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
#else /* PHASE2_SEPARATE_RC */
		wlcsm_mngr_restart(radioIndex, WLCSM_MNGR_RESTART_HTTPD, WLCSM_MNGR_RESTART_NOSAVEDM, 1);
#endif /* PHASE2_SEPARATE_RC */

		if (wl_UpTime(radioIndex, CMD_UPTIME_INIT, &initSecs) < 0) {
			WIFI_ERR("%s ACTION_SYS_RESTART Err init radioUpTime\n", __FUNCTION__);
		}

		/* Free the temporary object after being applied */
		wldm_free_RadioObject(index);
		return ret;
	}

	/* Update the applications */
	if (action & ACTION_APP_ACSD) {
		pid = get_pid_by_name("acsd2");
		if (pid > 0) {
			dm_unregister_app_restart_info(pid);
			if (system("killall -q -9 acsd2") == -1) {
				WIFI_ERR("%s: Couldn't issue killall -q -9 acsd2\n", __FUNCTION__);
			}
		}
		if (system("acsd2&") == -1) {
			WIFI_ERR("%s: Couldn't start acsd2 in the background\n", __FUNCTION__);
		}
		WIFI_DBG("%s: acsd restarted!\n", __FUNCTION__);
	}

	/* Break down into small actions to restart driver and apps */
	/* Update the driver */
	if (deferred_acts & ACTION_DRV_WLCONF) {
		/* Wlconf is deferred and will be called later, no need to issue IOCTLs either */
	} else if (action & ACTION_DRV_WLCONF) {
		snprintf(buf, sizeof(buf), "wlconf %s up", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
		snprintf(buf, sizeof(buf), "wlconf %s start", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
	} else if (ioctl_map || ioctl_map_XBrcm || ioctl_map_XRDK) {
		/* If we need to change the radio on the fly, get the lock and then issue ioctls */
		if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
			/* Aquire the driver lock locally */
			lfd = wldm_get_drvlock(radioIndex);
			if (lfd < 0) {
				WIFI_DBG("%s: No lock is aquired for radio %d!\n",
					__FUNCTION__, radioIndex);
			}
		}

		len = sizeof(enable);
		wldm_Radio_Enable(CMD_GET, index, &enable, &len, NULL, NULL);

		if (ioctl_wl_down && enable) {
			/* Make WL down and apply the configs. */
			enable = FALSE;
			len = sizeof(enable);
			wldm_Radio_Enable(CMD_SET_IOCTL, index,
				&enable, &len, NULL, NULL);
		}

		/* Issue IOCTLs according to ioctl_map. Do wl down/up if necessary.
		*  Rearrange the order below according to wl driver's behavior.
		*/
		if (ioctl_map & (Radio_OperatingChannelBandwidth_MASK |
			Radio_ExtensionChannel_MASK | Radio_Channel_MASK)) {
			if (pObj->apply_map & Radio_OperatingChannelBandwidth_MASK) {
				len = strlen(pObj->Radio.OperatingChannelBandwidth);
				wldm_Radio_OperatingChannelBandwidth(CMD_SET_IOCTL, index,
					pObj->Radio.OperatingChannelBandwidth, &len, NULL, NULL);
			}
			if (channel != 0) {
				len = sizeof(channel);
				wldm_Radio_Channel(CMD_SET_IOCTL, index,
					&channel, &len, bw, extChan, NULL, NULL);
			}
		}

		if (ioctl_map & Radio_BasicDataTransmitRates_MASK) {
			len = strlen(pObj->Radio.BasicDataTransmitRates) + 1;
			wldm_Radio_BasicDataTransmitRates(CMD_SET_IOCTL, index,
				pObj->Radio.BasicDataTransmitRates, &len, NULL, NULL);
		}

		if (ioctl_map & Radio_FragmentationThreshold_MASK) {
			len = sizeof(pObj->Radio.FragmentationThreshold);
			wldm_Radio_FragmentationThreshold(CMD_SET_IOCTL, index,
				&pObj->Radio.FragmentationThreshold, &len, NULL, NULL);
		}

		if (ioctl_map & Radio_IEEE80211hEnabled_MASK) {
			len = sizeof(pObj->Radio.IEEE80211hEnabled);
			wldm_Radio_IEEE80211hEnabled(CMD_SET_IOCTL, index,
				&pObj->Radio.IEEE80211hEnabled, &len, NULL, NULL);
		}

		if (ioctl_map & Radio_DTIMPeriod_MASK) {
			len = sizeof(pObj->Radio.DTIMPeriod);
			wldm_Radio_DTIMPeriod(CMD_SET_IOCTL, index,
				(int *) &pObj->Radio.DTIMPeriod, &len, NULL, NULL);
		}

		if (ioctl_map & Radio_GuardInterval_MASK) {
			len = strlen(pObj->Radio.GuardInterval) + 1;
			wldm_Radio_GuardInterval(CMD_SET_IOCTL, index,
				pObj->Radio.GuardInterval, &len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_AxEnable_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.AxEnable);
			wldm_AXenable(CMD_SET_IOCTL, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.AxEnable), &len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_AxFeatures_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.AxFeatures);
			wldm_AXfeatures(CMD_SET_IOCTL, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.AxFeatures), &len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_AxBsscolor_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.AxBsscolor);
			wldm_AXbssColor(CMD_SET_IOCTL, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.AxBsscolor), &len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_AxMuType_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.AxMuType);
			wldm_xbrcm_Radio_AXmuType(CMD_SET_IOCTL, index,
				(he_mu_type_t *)&(pObjXBrcm->X_BROADCOM_COM_Radio.AxMuType),
				(uint *)&len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_AxMuEdca_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca);
			wldm_xbrcm_Radio_AXmuEdca(CMD_SET_IOCTL, index,
				&pObjXBrcm->X_BROADCOM_COM_Radio.AxMuEdca,
				(uint *)&len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_STBCEnable_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.STBCEnable);
			wldm_xbrcm_Radio_STBCEnable(CMD_SET_IOCTL, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.STBCEnable), &len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_TxChainMask_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.TxChainMask);
			wldm_xbrcm_Radio_TxChainMask(CMD_SET_IOCTL, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.TxChainMask), &len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_RxChainMask_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.RxChainMask);
			wldm_xbrcm_Radio_RxChainMask(CMD_SET_IOCTL, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.RxChainMask), &len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_Greenfield11nEnable_MASK) {
			len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.Greenfield11nEnable);
			wldm_xbrcm_Radio_Greenfield11nEnable(CMD_SET_IOCTL, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.Greenfield11nEnable), &len, NULL, NULL);
		}

		if (ioctl_map_XRDK & X_RDK_Radio_AmsduEnable_MASK) {
			len = sizeof(pObjXRDK->X_RDK_Radio.amsdu);
			wldm_Radio_AMSDUEnable(CMD_SET_IOCTL, index,
				&(pObjXRDK->X_RDK_Radio.amsdu), &len, NULL, NULL);
		}

		if (ioctl_map & Radio_BeaconPeriod_MASK) {
			len = sizeof(pObj->Radio.BeaconPeriod);
			wldm_Radio_BeaconPeriod(CMD_SET_IOCTL, index,
				(int *) &pObj->Radio.BeaconPeriod, &len, NULL, NULL);
		}

		if (ioctl_map & Radio_AutoChannelRefreshPeriod_MASK) {
			len = sizeof(pObj->Radio.AutoChannelRefreshPeriod);
			wldm_Radio_AutoChannelRefreshPeriod(CMD_SET_IOCTL, index,
				&pObj->Radio.AutoChannelRefreshPeriod, &len, NULL, NULL);
		}

		if (ioctl_map & Radio_RegulatoryDomain_MASK) {
			len = strlen(pObj->Radio.RegulatoryDomain);
			wldm_Radio_RegulatoryDomain(CMD_SET_IOCTL, index,
				pObj->Radio.RegulatoryDomain, &len, NULL, NULL);
		}

		if (ioctl_map & Radio_MCS_MASK) {
			len = sizeof(pObj->Radio.MCS);
			wldm_Radio_MCS(CMD_SET_IOCTL, index,
				&pObj->Radio.MCS, &len, NULL, NULL);
		}

		if (ioctl_map_XRDK & X_RDK_Radio_AutoChannelDwellTime_MASK) {
			len = sizeof(pObjXRDK->X_RDK_Radio.AutoChannelDwellTime);
			wldm_Radio_AutoChannelDwellTime(CMD_SET_IOCTL, index,
				&pObjXRDK->X_RDK_Radio.AutoChannelDwellTime, &len, NULL, NULL);
		}

		if (ioctl_map_XRDK & X_RDK_Radio_DfsEnable_MASK) {
			len = sizeof(pObjXRDK->X_RDK_Radio.DfsEnable);
			wldm_Radio_DfsEnable(CMD_SET_IOCTL, index,
				&pObjXRDK->X_RDK_Radio.DfsEnable, &len, NULL, NULL);
		}

		if (ioctl_wl_down || (ioctl_map & Radio_Enable_MASK)) {
			/* Make wl up after all the configs are done only if Radio is enabled. */
			len = sizeof(radio_isup);
			wldm_Radio_Enable(CMD_GET_NVRAM, index,
				&radio_isup, &len, NULL, NULL);
			enable = (radio_isup) ? TRUE : FALSE;
			WIFI_DBG("%s: In nvram WiFi radio %s is set to be %s\n", __FUNCTION__,
				wl_ifname[radioIndex], enable ? "Up" : "Down");
			len = sizeof(enable);
			wldm_Radio_Enable(CMD_SET_IOCTL, index,	&enable, &len, NULL, NULL);
		}

		if (enable) {
			if (ioctl_map & Radio_TransmitPower_MASK) {
				len = sizeof(pObj->Radio.TransmitPower);
				wldm_Radio_TransmitPower(CMD_SET_IOCTL, index,
					&(pObj->Radio.TransmitPower), &len, NULL, NULL);
			}

			if (ioctl_map & Radio_RTSThreshold_MASK) {
				len = sizeof(pObj->Radio.RTSThreshold);
				wldm_Radio_RTSThreshold(CMD_SET_IOCTL, index,
					&pObj->Radio.RTSThreshold, &len, NULL, NULL);
			}

			if (ioctl_map_XBrcm & X_BROADCOM_COM_Radio_CtsProtectionEnable_MASK) {
				len = sizeof(pObjXBrcm->X_BROADCOM_COM_Radio.CtsProtectionEnable);
				wldm_xbrcm_Radio_CtsProtectionEnable(CMD_SET_IOCTL, index,
				&(pObjXBrcm->X_BROADCOM_COM_Radio.CtsProtectionEnable), &len, NULL, NULL);
			}

			if (ioctl_map & Radio_AutoChannelEnable_MASK) {
				len = sizeof(pObj->Radio.AutoChannelEnable);
				wldm_Radio_AutoChannelEnable(CMD_SET_IOCTL, index,
					&(pObj->Radio.AutoChannelEnable), &len, NULL, NULL);
			}
		}

		if (lfd >= 0) {
			/* Release the driver lock */
			wldm_free_lock(lfd);
		}
	}

	if (wl_UpTime(radioIndex, CMD_UPTIME_INIT, &initSecs) < 0) {
		WIFI_ERR("%s Err init radioUpTime\n", __FUNCTION__);
	}

	/* Free the temporary object after being applied */
	wldm_free_RadioObject(index);

	return ret;
}

int
wldm_apply_RadioObject(int radioIndex)
{
	int ret;

	RADIOINDEX_CHECK(radioIndex, -1);
	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_RadioObject(radioIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* SSID object */
int
wldm_free_SSIDObject(int ssidIndex)
{
	int index = ssidIndex;
	SSID_Object *pObj;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(index, -1);

	pObj = WLDM_CNTXT_GET(objSSID[index]);
	if (pObj == NULL)
		return 0;

	WLDM_CNTXT_SET(objSSID[index], NULL);

	/* Free the dynamically allocated RW contents(hexBinary, IPAddress, list, string) */
	if (pObj->Ssid.Alias) {
		free(pObj->Ssid.Alias);
		pObj->Ssid.Alias = NULL;
	}
	if (pObj->Ssid.LowerLayers) {
		free(pObj->Ssid.LowerLayers);
		pObj->Ssid.LowerLayers = NULL;
	}
	if (pObj->Ssid.SSID) {
		free(pObj->Ssid.SSID);
		pObj->Ssid.SSID = NULL;
	}

	free(pObj);
	return 0;
}

int
wldm_free_X_LGI_RatesControlObject(int ssidIndex)
{
	int index = ssidIndex;
	X_LGI_Rates_Bitmap_Control_Object *pObjX;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(index, -1);

	pObjX = WLDM_CNTXT_GET(objX_LGI_RatesBitmap[index]);
	if (pObjX == NULL)
		return 0;

	WLDM_CNTXT_SET(objX_LGI_RatesBitmap[index], NULL);

	if (pObjX->Bitmap.BasicRatesBitMap) {
		free(pObjX->Bitmap.BasicRatesBitMap);
		pObjX->Bitmap.BasicRatesBitMap = NULL;
	}

	if (pObjX->Bitmap.SupportedRatesBitMap) {
		free(pObjX->Bitmap.SupportedRatesBitMap);
		pObjX->Bitmap.SupportedRatesBitMap = NULL;
	}

	free(pObjX);
	return 0;
}

/*
*  Return the SSID_Object pointer if previously allocated, otherwise alloc a new one.
*  ssidIndex: AP Index starting from 0.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
SSID_Object *
wldm_get_SSIDObject(int ssidIndex, int checkMask)
{
	int index = ssidIndex;
	SSID_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, SSID_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objSSID[index], checkMask);

	return pObj;
}

X_LGI_Rates_Bitmap_Control_Object *
wldm_get_X_LGI_RatesControlObject(int ssidIndex, int checkMask)
{
	int index = ssidIndex;
	X_LGI_Rates_Bitmap_Control_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, X_LGI_RATE_CONTROL_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objX_LGI_RatesBitmap[index], checkMask);

	return pObj;
}

static int
apply_SSIDObject(int ssidIndex)
{
	int index = ssidIndex;
	int len, ret = 0;
	int action = 0, acts_to_defer, deferred_acts = 0;
	int ioctl_map = 0;	/* The map to configure driver by issuing ioctls. */
	int radioIndex = RADIO_INDEX(ssidIndex);
	SSID_Object *pObj;
	int lfd = -1;
	char nvram_name[128];
	char buf[256];
	char *osifname;
	WLDM_CNTXT_INIT(-1);

	pObj = WLDM_CNTXT_GET(objSSID[index]);
	if (pObj == NULL) {
		return 0;
	}

	if (pObj->reject_map) {
		WIFI_ERR("Invalid parameters are present, reject map 0x%0x!\n", pObj->reject_map);
		wldm_free_SSIDObject(index);
		return -2;
	}

	/* As of TR181 2.12, there are 4 RW parameters.
	*  Set the corresponding nvram variables, and determine how to restart.
	*/
	WIFI_DBG("%s: applying mapped 0x%0x params to %s: Device.WiFi.SSID.%d.\n",
		__FUNCTION__, pObj->apply_map, wl_ifname[index], index + 1);

	osifname = wldm_get_radio_osifname(radioIndex);
	if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
		/* No lock is done, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	if (pObj->apply_map & SSID_Alias_MASK) {
		WIFI_DBG("\tAlias=%s\n", pObj->Ssid.Alias);

		snprintf(nvram_name, sizeof(nvram_name), "%s_SSID.Alias", wl_ifname[index]);
		if (wlcsm_nvram_set(nvram_name, pObj->Ssid.Alias) != 0) {
			WIFI_DBG("%s: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, nvram_name, pObj->Ssid.Alias);
		}
		action |= ACTION_APP_NVRAM_COMMIT;
	}

	if (pObj->apply_map & SSID_LowerLayers_MASK) {
		WIFI_DBG("\tLowerLayers=%s\n", pObj->Ssid.LowerLayers);

		snprintf(nvram_name, sizeof(nvram_name), "%s_SSID.LowerLayers", wl_ifname[index]);
		/* For now we do not support dynamic interface stack to any radios */
		snprintf(buf, sizeof(buf), "Device.WiFi.Radio.%d.", radioIndex + 1);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_DBG("%s: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, nvram_name, buf);
		}
		action |= ACTION_APP_NVRAM_COMMIT;
	}

	if (pObj->apply_map & SSID_SSID_MASK) {
		WIFI_DBG("\tSSID=%s\n", pObj->Ssid.SSID);
		len = strlen(pObj->Ssid.SSID);
		if (wldm_SSID_SSID(CMD_SET_NVRAM, index, pObj->Ssid.SSID, &len, NULL, NULL) == 0) {
			if (HAPD_DISABLED())
				ioctl_map |= SSID_SSID_MASK;
			action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
		}
	}

	/* In hal avoid calling wldm_SSID_Enable() and use wldm_AccessPoint_Enable() */
	if (pObj->apply_map & SSID_Enable_MASK) {
		WIFI_DBG("\tEnable=%d\n", (int)pObj->Ssid.Enable);
		len = sizeof(pObj->Ssid.Enable);
		if (wldm_SSID_Enable(CMD_SET_NVRAM, index,
			&pObj->Ssid.Enable, &len, NULL, NULL) == 0) {
			if (HAPD_DISABLED())
				ioctl_map |= SSID_Enable_MASK;
			action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
		}
	}

	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	if (action & ACTION_SYS_RESTART) {
#ifdef PHASE2_SEPARATE_RC
		snprintf(buf, sizeof(buf), "wifi_setup.sh restart %s", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
#else /* PHASE2_SEPARATE_RC */
		wlcsm_mngr_restart(radioIndex, WLCSM_MNGR_RESTART_HTTPD, WLCSM_MNGR_RESTART_NOSAVEDM, 1);
#endif /* PHASE2_SEPARATE_RC */

		/* Free the temporary object after being applied */
		wldm_free_SSIDObject(index);
		return ret;
	}

	/* Break down into small actions to reconfig driver and apps */
	WIFI_DBG("\taction=%x ioctl_map=%x\n", action, ioctl_map);

	/* If we need to change the radio on the fly, get the lock and then issue ioctls */
	if (deferred_acts & ACTION_DRV_WLCONF) {
		/* Wlconf is deferred and will be called later, no need to issue IOCTLs either */
	} else if (action & ACTION_DRV_WLCONF) {
		snprintf(buf, sizeof(buf), "wlconf %s up", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
		snprintf(buf, sizeof(buf), "wlconf %s start", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
	} else if (ioctl_map) {
		/* If we need to change the radio on the fly, get the lock and then issue ioctls */
		if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
			/* Aquire the driver lock locally */
			lfd = wldm_get_drvlock(radioIndex);
			if (lfd < 0) {
				WIFI_DBG("%s: No lock is aquired for radio %d!\n",
					__FUNCTION__, radioIndex);
			}
		}

		/* Issue IOCTLs according to ioctl_map. Do wl down/up if necessary.
		*  Rearrange the order below according to wl driver's behavior.
		*/
		if (ioctl_map & SSID_SSID_MASK) {
			len = strlen(pObj->Ssid.SSID);
			wldm_SSID_SSID(CMD_SET_IOCTL, index, pObj->Ssid.SSID, &len, NULL, NULL);
		}

		if (ioctl_map & SSID_Enable_MASK) {
			len = sizeof(pObj->Ssid.Enable);
			wldm_SSID_Enable(CMD_SET_IOCTL, index,
				&pObj->Ssid.Enable, &len, NULL, NULL);
		}

		if (lfd >= 0) {
			/* Release the driver lock */
			wldm_free_lock(lfd);
		}
	}

	if (action & ACTION_APP_HOSTAPD) {
		WIFI_DBG("%s: restart security daemons %d\n", __FUNCTION__, radioIndex);
		restart_wsec_daemons(radioIndex);
	}

	/* Free the temporary object after being applied */
	wldm_free_SSIDObject(index);

	return ret;
}

int
wldm_apply_SSIDObject(int ssidIndex)
{
	int ret;

	APINDEX_CHECK(ssidIndex, -1);
	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_SSIDObject(ssidIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

static int
apply_X_LGI_RatesControlObject(int ssidIndex)
{
	int index = ssidIndex;
	int len, ret = 0, lfd = -1;
	int action = 0, acts_to_defer, deferred_acts = 0;
	int ioctl_map = 0;	/* The map to configure driver by issuing ioctls. */
	int ioctl_wl_down = 0;
	bool radio_isup, enable;
	int radioIndex = RADIO_INDEX(ssidIndex);
	X_LGI_Rates_Bitmap_Control_Object *pObj;
	char buf[256];
	char *osifname;
	WLDM_CNTXT_INIT(-1);

	pObj = WLDM_CNTXT_GET(objX_LGI_RatesBitmap[index]);
	if (pObj == NULL) {
		return 0;
	}

	if (pObj->reject_map) {
		WIFI_ERR("Invalid parameters are present, reject map 0x%0x!\n", pObj->reject_map);
		wldm_free_X_LGI_RatesControlObject(index);
		return -2;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	WIFI_DBG("%s: applying mapped 0x%0x params to %s:"
		"Device.WiFi.SSID.%d.X_LGI-COM_WiFiSupportedRates\n",
		__FUNCTION__, pObj->apply_map, wl_ifname[index], index + 1);

	if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
		/* No lock is done, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	if (pObj->apply_map & X_LGI_RATE_CONTROL_Enable_MASK) {
		WIFI_DBG("\tRateBitMapControlEnable=%d\n", pObj->Bitmap.Enable);
		len = sizeof(pObj->Bitmap.Enable);
		if (wldm_RatesBitmapControl_Enable(CMD_SET_NVRAM,
			&(pObj->Bitmap.Enable), &len, NULL, NULL) == 0) {
			action |= ACTION_APP_NVRAM_COMMIT;
		}
	}

	if (pObj->apply_map & X_LGI_RATE_CONTROL_SupportRate_MASK) {
		WIFI_DBG("\tSupportRatesBitMap=%s\n", pObj->Bitmap.SupportedRatesBitMap);
		len = strlen(pObj->Bitmap.SupportedRatesBitMap);
		if (wldm_RatesBitmapControl_SupportedRate(CMD_SET_NVRAM, index,
			(pObj->Bitmap.SupportedRatesBitMap), &len, NULL, NULL) == 0) {
			action |= ACTION_APP_NVRAM_COMMIT;
			ioctl_map |= X_LGI_RATE_CONTROL_SupportRate_MASK;
			ioctl_wl_down = 1;
		}
	}

	if (pObj->apply_map & X_LGI_RATE_CONTROL_BasicRate_MASK) {
		WIFI_DBG("\tBasicRatesBitMap=%s\n", pObj->Bitmap.BasicRatesBitMap);
		len = strlen(pObj->Bitmap.BasicRatesBitMap);
		if (wldm_RatesBitmapControl_BasicRate(CMD_SET_NVRAM, index,
			(pObj->Bitmap.BasicRatesBitMap), &len, NULL, NULL) == 0) {
			action |= ACTION_APP_NVRAM_COMMIT;
			ioctl_map |= X_LGI_RATE_CONTROL_BasicRate_MASK;
			ioctl_wl_down = 1;
		}
	}

	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	/* Break down into small actions to reconfig driver and apps */
	WIFI_DBG("\taction=%x ioctl_map=%x\n", action, ioctl_map);

	/* If we need to change the radio on the fly, get the lock and then issue ioctls */
	if (deferred_acts & ACTION_DRV_WLCONF) {
		/* Wlconf is deferred and will be called later, no need to issue IOCTLs either */
	} else if (action & ACTION_DRV_WLCONF) {
		snprintf(buf, sizeof(buf), "wlconf %s up", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
		snprintf(buf, sizeof(buf), "wlconf %s start", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
	} else if (ioctl_map) {
		/* If we need to change the radio on the fly, get the lock and then issue ioctls */
		if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
			/* Aquire the driver lock locally */
			lfd = wldm_get_drvlock(radioIndex);
			if (lfd < 0) {
				WIFI_DBG("%s: No lock is aquired for radio %d!\n",
					__FUNCTION__, radioIndex);
			}
		}

		if (ioctl_wl_down) {
			/* Make WL down and apply the configs. */
			enable = FALSE;
			len = sizeof(enable);
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,
				&enable, &len, NULL, NULL);
		}

		if (ioctl_map & X_LGI_RATE_CONTROL_SupportRate_MASK) {
			WIFI_DBG("\tSupportRatesBitMap set ioctl called\n");
			len = strlen(pObj->Bitmap.SupportedRatesBitMap);
			wldm_RatesBitmapControl_SupportedRate(CMD_SET_IOCTL, index,
				pObj->Bitmap.SupportedRatesBitMap, &len, NULL, NULL);
		}

		if (ioctl_map & X_LGI_RATE_CONTROL_BasicRate_MASK) {
			WIFI_DBG("\tBasicRatesBitMap set ioctl called\n");
			len = strlen(pObj->Bitmap.BasicRatesBitMap);
			wldm_RatesBitmapControl_BasicRate(CMD_SET_IOCTL, index,
				pObj->Bitmap.BasicRatesBitMap, &len, NULL, NULL);
		}
		if (ioctl_wl_down) {
			/* Make wl up after all the configs are done only if Radio is enabled. */
			len = sizeof(radio_isup);
			wldm_Radio_Enable(CMD_GET_NVRAM, radioIndex,
				&radio_isup, &len, NULL, NULL);
			if (radio_isup) {
				WIFI_DBG("%s: In nvram WiFi radio %s is set to be UP\n",
					__FUNCTION__, wl_ifname[radioIndex]);
				enable = TRUE;
				len = sizeof(enable);
				wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,
					&enable, &len, NULL, NULL);
			} else {
				WIFI_DBG("%s: In nvram WiFi radio %s is set to be Down\n",
					__FUNCTION__, wl_ifname[radioIndex]);
			}
		}

		if (lfd >= 0) {
			/* Release the driver lock */
			wldm_free_lock(lfd);
		}
	}

	/* Free the temporary object after being applied */
	wldm_free_X_LGI_RatesControlObject(index);

	return ret;
}

int
wldm_apply_X_LGI_RatesControlObject(int ssidIndex)
{
	int ret;

	APINDEX_CHECK(ssidIndex, -1);
	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_X_LGI_RatesControlObject(ssidIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* AccessPoint object */
/*
*  Free the contents and the object.
*  apIndex: AP Index starting from 0
*/
int
wldm_free_AccessPointObject(int apIndex)
{
	int index = apIndex;
	AccessPoint_Object *pObj;
	X_RDK_AccessPoint_Object *pObjXRdk;
	X_BROADCOM_COM_AccessPoint_Object *pObjXBrcm;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(index, -1);

	pObj = WLDM_CNTXT_GET(objAccessPoint[index]);
	pObjXRdk = WLDM_CNTXT_GET(objX_RDK_AccessPoint[index]);
	pObjXBrcm = WLDM_CNTXT_GET(objX_BROADCOM_COM_AccessPoint[index]);

	WLDM_CNTXT_SET(objAccessPoint[index], NULL);
	WLDM_CNTXT_SET(objX_RDK_AccessPoint[index], NULL);
	WLDM_CNTXT_SET(objX_BROADCOM_COM_AccessPoint[index], NULL);

	if (pObj) {
		/* Free the dynamically allocated RW contents(hexBinary, IPAddress, list, string) */
		if (pObj->Ap.Alias) {
			free(pObj->Ap.Alias);
			pObj->Ap.Alias = NULL;
		}
		if (pObj->Ap.SSIDReference) {
			free(pObj->Ap.SSIDReference);
			pObj->Ap.SSIDReference = NULL;
		}
		if (pObj->Ap.AllowedMACAddress) {
			free(pObj->Ap.AllowedMACAddress);
			pObj->Ap.AllowedMACAddress = NULL;
		}

		free(pObj);
	}

	if (pObjXRdk != NULL) {
		if (pObjXRdk->Ap.MACAddresslist) {
			free(pObjXRdk->Ap.MACAddresslist);
			pObjXRdk->Ap.MACAddresslist = NULL;
		}
		free(pObjXRdk);
	}

	if (pObjXBrcm != NULL) {
		free(pObjXBrcm);
	}

	return 0;
}

/*
*  Return the AccessPoint_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index starting from 0.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
AccessPoint_Object *
wldm_get_AccessPointObject(int apIndex, int checkMask)
{
	int index = apIndex;
	AccessPoint_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, AccessPoint_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objAccessPoint[index], checkMask);

	return pObj;
}

/*
*  Return the X_RDK_AccessPoint_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index starting from 0.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
X_RDK_AccessPoint_Object *
wldm_get_X_RDK_AccessPointObject(int apIndex, int checkMask)
{
	int index = apIndex;
	X_RDK_AccessPoint_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, X_RDK_AccessPoint_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objX_RDK_AccessPoint[index], checkMask);

	return pObj;
}

/*
*  Return the X_BROADCOM_COM_AccessPoint_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index starting from 0.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
X_BROADCOM_COM_AccessPoint_Object *
wldm_get_X_BROADCOM_COM_AccessPointObject(int apIndex, int checkMask)
{
	int index = apIndex;
	X_BROADCOM_COM_AccessPoint_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, X_BROADCOM_COM_AccessPoint_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objX_BROADCOM_COM_AccessPoint[index], checkMask);

	return pObj;
}

static int
apply_AccessPointObject(int apIndex)
{
	int index = apIndex;
	int len, ret = 0;
	int action = 0, acts_to_defer, deferred_acts = 0;
	int ioctl_map = 0, ioctl_map_XRdk = 0, ioctl_map_XBrcm = 0;	/* The map to configure driver by issuing ioctls. */
	int radioIndex = RADIO_INDEX(apIndex);
	int lfd = -1;
	AccessPoint_Object *pObj;
	X_RDK_AccessPoint_Object *pObjXRdk;
	X_BROADCOM_COM_AccessPoint_Object *pObjXBrcm;
	char nvram_name[128];
	char buf[256];
	char *osifname;
	int radio_isup, ioctl_wl_down = 0, enable;
	WLDM_CNTXT_INIT(-1);

	WIFI_INFO("%s apIndex=%d Enter\n", __FUNCTION__, apIndex);

	pObj = WLDM_CNTXT_GET(objAccessPoint[index]);
	pObjXRdk = WLDM_CNTXT_GET(objX_RDK_AccessPoint[index]);
	pObjXBrcm = WLDM_CNTXT_GET(objX_BROADCOM_COM_AccessPoint[index]);
	if ((pObj == NULL) && (pObjXRdk == NULL) && (pObjXBrcm == NULL)) {
		return 0;
	}

	if (pObjXRdk && pObjXRdk->reject_map) {
		WIFI_DBG("Invalid parameters are present in X_RDK_AccessPoint_Object, reject map 0x%0x!\n",
			pObjXRdk->reject_map);
	}

	if (pObjXBrcm && pObjXBrcm->reject_map) {
		WIFI_DBG("Invalid parameters are present in X_BROADCOM_COM_AccessPoint_Object, reject map 0x%0x!\n",
			pObjXRdk->reject_map);
	}

	if (pObj && pObj->reject_map) {
		WIFI_ERR("%s: invalid parameters are present, reject map 0x%0x!\n",
			__FUNCTION__, pObj->reject_map);
		wldm_free_AccessPointObject(index);
		return -2;
	}

	/* As of TR181 2.12, there are 4 RW parameters.
	*  Set the corresponding nvram variables, and determine how to restart.
	*/
	osifname = wldm_get_radio_osifname(radioIndex);

	if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
		/* No lock is done, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	if (pObj) {
		WIFI_DBG("%s: applying mapped 0x%0x params to %s: Device.WiFi.AccessPoint.%d.\n",
			__FUNCTION__, pObj->apply_map, wldm_get_osifname(index), index + 1);
		if (pObj->apply_map & AccessPoint_Enable_MASK) {
			WIFI_DBG("\tEnable=%d\n", (int)pObj->Ap.Enable);
			len = sizeof(pObj->Ap.Enable);
			if (wldm_AccessPoint_Enable(CMD_SET_NVRAM, index,
				&pObj->Ap.Enable, &len, NULL, NULL) == 0) {
				if (HAPD_DISABLED()) {
					ioctl_map |= AccessPoint_Enable_MASK;
				}
#ifdef BCA_CPEROUTER_RDK
				if (BSS_IDX(apIndex))
					action |= ACTION_APP_NVRAM_COMMIT | ACTION_SYS_RESTART;
				else
#endif
				action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
			}
		}

		if (pObj->apply_map & AccessPoint_Alias_MASK) {
			WIFI_DBG("\tAlias=%s\n", pObj->Ap.Alias);

			snprintf(nvram_name, sizeof(nvram_name), "%s_AccessPoint.Alias", wl_ifname[index]);
			if (wlcsm_nvram_set(nvram_name, pObj->Ap.Alias)) {
				WIFI_DBG("%s: wlcsm_nvram_set %s=%s failed!\n",
					__FUNCTION__, nvram_name, pObj->Ap.Alias);
			}
			action |= ACTION_APP_NVRAM_COMMIT;
		}

		if (pObj->apply_map & AccessPoint_SSIDReference_MASK) {
			WIFI_DBG("\tSSIDReference=%s\n", pObj->Ap.SSIDReference);

			snprintf(nvram_name, sizeof(nvram_name), "%s_AccessPoint.SSIDReference",
				wl_ifname[index]);
			/* For now we do not support dynamic interface stack to any SSID */
			snprintf(buf, sizeof(buf), "Device.WiFi.SSID.%d.", index + 1);
			if (wlcsm_nvram_set(nvram_name, buf)) {
				WIFI_DBG("%s: wlcsm_nvram_set %s=%s failed!\n",
					__FUNCTION__, nvram_name, buf);
			}
			action |= ACTION_APP_NVRAM_COMMIT;
		}

		if (pObj->apply_map & AccessPoint_SSIDAdvertisementEnabled_MASK) {
			WIFI_DBG("\tSSIDAdvertisementEnabled=%d\n", (int)pObj->Ap.SSIDAdvertisementEnabled);
			len = sizeof(pObj->Ap.SSIDAdvertisementEnabled);
			if (wldm_AccessPoint_SSIDAdvertisementEnabled(CMD_SET_NVRAM, index,
				&pObj->Ap.SSIDAdvertisementEnabled, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				if (HAPD_DISABLED())
					ioctl_map |= AccessPoint_SSIDAdvertisementEnabled_MASK;
				else
					action |= ACTION_APP_HOSTAPD;
			}
		}

		if (pObj->apply_map & AccessPoint_RetryLimit_MASK) {
			WIFI_DBG("\tAccessPoint_RetryLimit=%d\n", (int)pObj->Ap.RetryLimit);
			len = sizeof(pObj->Ap.RetryLimit);
			ioctl_map |= AccessPoint_RetryLimit_MASK;
		}

		if (pObj->apply_map & AccessPoint_WMMEnable_MASK) {
			WIFI_DBG("\tAccessPoint_WMMEnable=%d\n", (int)pObj->Ap.WMMEnable);
			len = sizeof(pObj->Ap.WMMEnable);
			if (wldm_AccessPoint_WMMEnable(CMD_SET_NVRAM, index,
				&pObj->Ap.WMMEnable, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= AccessPoint_WMMEnable_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObj->apply_map & AccessPoint_UAPSDEnable_MASK) {
			WIFI_DBG("\tAccessPoint_UAPSDEnable=%d\n", (int)pObj->Ap.UAPSDEnable);
			len = sizeof(pObj->Ap.UAPSDEnable);
			if (wldm_AccessPoint_UAPSDEnable(CMD_SET_NVRAM, index,
				&pObj->Ap.UAPSDEnable, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= AccessPoint_UAPSDEnable_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObj->apply_map & AccessPoint_MaxAssociatedDevices_MASK) {
			WIFI_DBG("\tAccessPoint_MaxAssociatedDevices=%d\n",
				(int)pObj->Ap.MaxAssociatedDevices);
			len = sizeof(pObj->Ap.MaxAssociatedDevices);
			if (wldm_AccessPoint_MaxAssociatedDevices(CMD_SET_NVRAM, index,
				(int *) &pObj->Ap.MaxAssociatedDevices, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= AccessPoint_MaxAssociatedDevices_MASK;
			}
		}

		if (pObj->apply_map & AccessPoint_IsolationEnable_MASK) {
			WIFI_DBG("\tAccessPoint_IsolationEnable=%d\n", (int)pObj->Ap.IsolationEnable);
			len = sizeof(pObj->Ap.IsolationEnable);
			if (wldm_AccessPoint_IsolationEnable(CMD_SET_NVRAM, index,
				&pObj->Ap.IsolationEnable, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				if (HAPD_DISABLED())
					ioctl_map |= AccessPoint_IsolationEnable_MASK;
				else
					action |= ACTION_APP_HOSTAPD;
			}
		}

		if (pObj->apply_map & AccessPoint_MACAddressControlEnabled_MASK) {
			WIFI_DBG("\tAccessPoint_MACAddressControlEnabled=%d\n",
				(int)pObj->Ap.MACAddressControlEnabled);
		}

		if (pObj->apply_map & AccessPoint_AllowedMACAddress_MASK) {
			WIFI_DBG("\tAccessPoint_AllowedMACAddress=%d\n", (int)pObj->Ap.AllowedMACAddress);
		}

		if (pObj->apply_map & AccessPoint_MaxAllowedAssociations_MASK) {
			WIFI_DBG("\tAccessPoint_MaxAllowedAssociations=%d\n",
				(int)pObj->Ap.MaxAllowedAssociations);
		}
	}

	if (pObjXRdk)
	{
		if (pObjXRdk->apply_map & X_RDK_AccessPoint_MACAddressControMode_MASK) {
			WIFI_DBG("\tMACAddressControMode=%d\n", (int)pObjXRdk->Ap.MACAddressControMode);
			len = sizeof(pObjXRdk->Ap.MACAddressControMode);
			if (wldm_AccessPoint_MACAddressControMode(CMD_SET_NVRAM, index,
				(int *) &pObjXRdk->Ap.MACAddressControMode, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XRdk |= X_RDK_AccessPoint_MACAddressControMode_MASK;
			 }
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_MACAddresslist_MASK) {
			WIFI_DBG("\tMACAddresslist=%d\n", (int)pObjXRdk->Ap.MACAddresslist);
			len = sizeof(pObjXRdk->Ap.MACAddresslist);
			if (wldm_AccessPoint_AclDevice(CMD_SET_NVRAM, index,
				pObjXRdk->Ap.MACAddresslist, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map_XRdk |= X_RDK_AccessPoint_MACAddresslist_MASK;
			}
		}
	}

	if (pObjXBrcm) {
		if (pObjXBrcm->apply_map & X_BROADCOM_COM_AccessPoint_RMCapabilities_MASK) {
                        unsigned char *rmcap = pObjXBrcm->Ap.RMCapabilities;
                        len = sizeof(pObjXBrcm->Ap.RMCapabilities);
                        WIFI_DBG("\tRMCapabilities="RMCAPF" len=%d\n", RMCAP_TO_RMCAPF(rmcap), len);
                        if (wldm_xbrcm_AccessPoint_RMCapabilities(CMD_SET_NVRAM, index,
                                pObjXBrcm->Ap.RMCapabilities, &len, NULL, NULL) == 0) {
                                action |= ACTION_APP_NVRAM_COMMIT;
                                ioctl_map_XBrcm |= X_BROADCOM_COM_AccessPoint_RMCapabilities_MASK;
                                ioctl_wl_down = 1;
                        }
		}
	}

	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	if (action & ACTION_SYS_RESTART) {
#ifdef PHASE2_SEPARATE_RC
		snprintf(buf, sizeof(buf), "wifi_setup.sh restart %s", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
#else /* PHASE2_SEPARATE_RC */
		wlcsm_mngr_restart(radioIndex, WLCSM_MNGR_RESTART_HTTPD, WLCSM_MNGR_RESTART_NOSAVEDM, 1);
#endif /* PHASE2_SEPARATE_RC */

		/* Free the temporary object after being applied */
		wldm_free_AccessPointObject(index);
		return ret;
	}

	/* Break down into small actions to reconfig driver and apps */
	WIFI_DBG("\taction=%x ioctl_map=%x ioctl_map_XRdk=%x\n",
		action, ioctl_map, ioctl_map_XRdk);

	/* Restart security daemons. */
	if (action & ACTION_APP_HOSTAPD) {
		WIFI_DBG("%s: restart security daemons\n", __FUNCTION__);
		restart_wsec_daemons(radioIndex);
	}

	/* If we need to change the radio on the fly, get the lock and then issue ioctls */
	if (deferred_acts & ACTION_DRV_WLCONF) {
		/* Wlconf is deferred and will be called later, no need to issue IOCTLs either */
	} else if (action & ACTION_DRV_WLCONF) {
		snprintf(buf, sizeof(buf), "wlconf %s up", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
		snprintf(buf, sizeof(buf), "wlconf %s start", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
	} else if (ioctl_map || ioctl_map_XRdk || ioctl_map_XBrcm) {
		/* If we need to change the radio on the fly, get the lock and then issue ioctls */
		if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
			/* Aquire the driver lock locally */
			lfd = wldm_get_drvlock(radioIndex);
			if (lfd < 0) {
				WIFI_DBG("%s: No lock is aquired for radio %d!\n",
					__FUNCTION__, radioIndex);
			}
		}

		if (ioctl_wl_down) {
			/* Make WL down and apply the configs. */
			enable = 0;
			len = sizeof(enable);
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,
				(boolean *)&enable, &len, NULL, NULL);
		}
		/* Issue IOCTLs according to ioctl_map. Do wl down/up if necessary.
		*  Rearrange the order below according to wl driver's behavior.
		*/

		if (ioctl_map & AccessPoint_SSIDAdvertisementEnabled_MASK) {
			len = sizeof(pObj->Ap.SSIDAdvertisementEnabled);
			wldm_AccessPoint_SSIDAdvertisementEnabled(CMD_SET_IOCTL, index,
				&pObj->Ap.SSIDAdvertisementEnabled, &len, NULL, NULL);
		}

		if (ioctl_map & AccessPoint_IsolationEnable_MASK) {
			len = sizeof(pObj->Ap.IsolationEnable);
			wldm_AccessPoint_IsolationEnable(CMD_SET_IOCTL, index,
				&pObj->Ap.IsolationEnable, &len, NULL, NULL);
		}

		if (ioctl_map & AccessPoint_RetryLimit_MASK) {
			len = sizeof(pObj->Ap.RetryLimit);
			wldm_AccessPoint_RetryLimit(CMD_SET_IOCTL, index,
			&pObj->Ap.RetryLimit, &len, NULL, NULL);
		}

		if (ioctl_map & AccessPoint_Enable_MASK) {
			len = sizeof(pObj->Ap.Enable);
			wldm_AccessPoint_Enable(CMD_SET_IOCTL, index,
				&pObj->Ap.Enable, &len, NULL, NULL);
		}

		if (ioctl_map_XRdk & X_RDK_AccessPoint_MACAddressControMode_MASK) {
			len = sizeof(pObjXRdk->Ap.MACAddressControMode);
			wldm_AccessPoint_MACAddressControMode(CMD_SET_IOCTL, index,
				(int *) &pObjXRdk->Ap.MACAddressControMode, &len, NULL, NULL);
		}

		if (ioctl_map_XRdk & X_RDK_AccessPoint_MACAddresslist_MASK) {
			len = sizeof(pObjXRdk->Ap.MACAddresslist);
			wldm_AccessPoint_AclDevice(CMD_SET_IOCTL, index,
				pObjXRdk->Ap.MACAddresslist, &len, NULL, NULL);
		}

		if (ioctl_map & AccessPoint_WMMEnable_MASK) {
			len = sizeof(pObj->Ap.WMMEnable);
			wldm_AccessPoint_WMMEnable(CMD_SET_IOCTL, index,
				&pObj->Ap.WMMEnable, &len, NULL, NULL);
		}

		if (ioctl_map & AccessPoint_UAPSDEnable_MASK) {
			len = sizeof(pObj->Ap.UAPSDEnable);
			wldm_AccessPoint_UAPSDEnable(CMD_SET_IOCTL, index,
				&pObj->Ap.UAPSDEnable, &len, NULL, NULL);
		}

		if (ioctl_map & AccessPoint_MaxAssociatedDevices_MASK) {
			len = sizeof(pObj->Ap.MaxAssociatedDevices);
			wldm_AccessPoint_MaxAssociatedDevices(CMD_SET_IOCTL, index,
				(int *) &pObj->Ap.MaxAssociatedDevices, &len, NULL, NULL);
		}

		if (ioctl_map_XBrcm & X_BROADCOM_COM_AccessPoint_RMCapabilities_MASK) {
			len = sizeof(pObjXBrcm->Ap.RMCapabilities);
			wldm_xbrcm_AccessPoint_RMCapabilities(CMD_SET_IOCTL, index,
				pObjXBrcm->Ap.RMCapabilities, &len, NULL, NULL);
		}

		if (ioctl_wl_down) {
			/* Make wl up after all the configs are done only if Radio is enabled. */
			len = sizeof(radio_isup);
			wldm_Radio_Enable(CMD_GET_NVRAM, radioIndex,
				(boolean *)&radio_isup, &len, NULL, NULL);
			if (radio_isup) {
				WIFI_DBG("%s %d: In nvram WiFi radio %s is set to be UP\n",
					__FUNCTION__, __LINE__, wl_ifname[radioIndex]);
				enable = 1;
				len = sizeof(enable);
				wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,
					(boolean *)&enable, &len, NULL, NULL);
			} else {
				WIFI_DBG("%s %d: In nvram WiFi radio %s is set to be Down\n",
					__FUNCTION__, __LINE__, wl_ifname[radioIndex]);
			}
		}

		if (lfd >= 0) {
			/* Release the driver lock */
			wldm_free_lock(lfd);
		}
	}

	/* Free the temporary object after being applied */
	wldm_free_AccessPointObject(index);

	return ret;
}

int
wldm_apply_AccessPointObject(int apIndex)
{
	int ret;

	APINDEX_CHECK(apIndex, -1);
	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_AccessPointObject(apIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* AccessPoint_Security object */
/*
*  Free the contents and the object.
*  apIndex: AP Index.
*/
int
wldm_free_AccessPointSecurityObject(int apIndex)
{
	int index = apIndex;
	AccessPoint_Security_Object *pObj;
	X_RDK_AccessPoint_Security_Object *pObjXRdk;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(index, -1);

	pObj = WLDM_CNTXT_GET(objAccessPoint_Security[index]);
	pObjXRdk = WLDM_CNTXT_GET(objX_RDK_AccessPoint_Security[index]);

	WLDM_CNTXT_SET(objAccessPoint_Security[index], NULL);
	WLDM_CNTXT_SET(objX_RDK_AccessPoint_Security[index], NULL);
	if (pObj) {
		/* Free the dynamically allocated RW contents(hexBinary, IPAddress, list, string) */
		if (pObj->Security.ModeEnabled) {
			free(pObj->Security.ModeEnabled);
			pObj->Security.ModeEnabled = NULL;
		}
		if (pObj->Security.WEPKey) {
			free(pObj->Security.WEPKey);
			pObj->Security.WEPKey = NULL;
		}
		if (pObj->Security.PreSharedKey) {
			free(pObj->Security.PreSharedKey);
			pObj->Security.PreSharedKey = NULL;
		}
		if (pObj->Security.KeyPassphrase) {
			free(pObj->Security.KeyPassphrase);
			pObj->Security.KeyPassphrase = NULL;
		}
		if (pObj->Security.RadiusServerIPAddr) {
			free(pObj->Security.RadiusServerIPAddr);
			pObj->Security.RadiusServerIPAddr = NULL;
		}
		if (pObj->Security.SecondaryRadiusServerIPAddr) {
			free(pObj->Security.SecondaryRadiusServerIPAddr);
			pObj->Security.SecondaryRadiusServerIPAddr = NULL;
		}
		if (pObj->Security.RadiusSecret) {
			free(pObj->Security.RadiusSecret);
			pObj->Security.RadiusSecret = NULL;
		}
		if (pObj->Security.SecondaryRadiusSecret) {
			free(pObj->Security.SecondaryRadiusSecret);
			pObj->Security.SecondaryRadiusSecret = NULL;
		}
		if (pObj->Security.MFPConfig) {
			free(pObj->Security.MFPConfig);
			pObj->Security.MFPConfig = NULL;
		}

		free(pObj);
	}

	if (pObjXRdk != NULL) {
		if (pObjXRdk->Security.BasicAuthentication) {
			free(pObjXRdk->Security.BasicAuthentication);
			pObjXRdk->Security.BasicAuthentication = NULL;
		}
		if (pObjXRdk->Security.Encryption) {
			free(pObjXRdk->Security.Encryption);
			pObjXRdk->Security.Encryption = NULL;
		}
		if (pObjXRdk->Security.RadiusOperatorName) {
			free(pObjXRdk->Security.RadiusOperatorName);
			pObjXRdk->Security.RadiusOperatorName = NULL;
		}
		if (pObjXRdk->Security.RadiusLocationData) {
			free(pObjXRdk->Security.RadiusLocationData);
			pObjXRdk->Security.RadiusLocationData = NULL;
		}
		if (pObjXRdk->Security.RadiusDASClientIPAddr) {
			free(pObjXRdk->Security.RadiusDASClientIPAddr);
			pObjXRdk->Security.RadiusDASClientIPAddr = NULL;
		}
		if (pObjXRdk->Security.RadiusDASSecret) {
			free(pObjXRdk->Security.RadiusDASSecret);
			pObjXRdk->Security.RadiusDASSecret = NULL;
		}
		free(pObjXRdk);
	}

	return 0;
}

/*
*  Return the AccessPointSecurity_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
AccessPoint_Security_Object *
wldm_get_AccessPointSecurityObject(int apIndex, int checkMask)
{
	int index = apIndex;
	AccessPoint_Security_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, AccessPoint_Security_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objAccessPoint_Security[index], checkMask);

	return pObj;
}

/*
*  Return the X_RDK_AccessPoint_Security_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
X_RDK_AccessPoint_Security_Object *
wldm_get_X_RDK_AccessPointSecurityObject(int apIndex, int checkMask)
{
	int index = apIndex;
	X_RDK_AccessPoint_Security_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, X_RDK_AccessPoint_Security_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objX_RDK_AccessPoint_Security[index], checkMask);

	return pObj;
}

static int
apply_AccessPointSecurityObject(int apIndex)
{
	int index = apIndex;
	int len, ret = 0, enable, radio_isup;
	int action = ACTION_APP_HOSTAPD, acts_to_defer, deferred_acts = 0;
	int radioIndex = RADIO_INDEX(apIndex);
	int lfd = -1;
	AccessPoint_Security_Object *pObj;
	X_RDK_AccessPoint_Security_Object *pObjXRdk;
	int ioctl_map = 0, ioctl_map_XRdk = 0, ioctl_wl_down = 0; /* The map to configure driver by issuing ioctls. */
	char buf[256], *osifname;
	WLDM_CNTXT_INIT(-1);

	osifname = wldm_get_radio_osifname(radioIndex);
	pObj = WLDM_CNTXT_GET(objAccessPoint_Security[index]);
	pObjXRdk = WLDM_CNTXT_GET(objX_RDK_AccessPoint_Security[index]);
	if ((pObj == NULL) && (pObjXRdk == NULL)) {
		return 0;
	}

	if (pObjXRdk && pObjXRdk->reject_map) {
		WIFI_DBG("Invalid parameters are present in X_RDK_AccessPoint_Security_Object, reject map 0x%0x!\n",
			pObjXRdk->reject_map);
	}

	if (pObj && pObj->reject_map) {
		WIFI_ERR("%s: invalid parameters are present, reject map 0x%0x!\n",
			__FUNCTION__, pObj->reject_map);
		wldm_free_AccessPointSecurityObject(index);
		return -2;
	}

	/* As of TR181 2.12, there are 13 RW parameters.
	*  Set the corresponding nvram variables, and determine how to restart.
	*/
	if (pObj) {
		WIFI_DBG("%s: applying mapped 0x%0x params to %s: Device.WiFi.AccessPoint.%d.Security.\n",
			__FUNCTION__, pObj->apply_map, wl_ifname[index], index + 1);
	}

	if (WLDM_GET_LOCK(nvram_lockfd) < 0) {
		/* No lock is done, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	if (pObj) {
		if (pObj->apply_map & AccessPoint_Security_ModeEnabled_MASK) {
			WIFI_DBG("\tModeEnabled=%s\n", pObj->Security.ModeEnabled);
			len = strlen(pObj->Security.ModeEnabled) + 1;
			if (wldm_AccessPoint_Security_ModeEnabled(CMD_SET_NVRAM, index,
				pObj->Security.ModeEnabled, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
				if (HAPD_DISABLED())
					ioctl_map |= AccessPoint_Security_ModeEnabled_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_WEPKey_MASK) {
			WIFI_DBG("\tWEPKey=%s\n", pObj->Security.WEPKey);
		}

		if (pObj->apply_map & AccessPoint_Security_PreSharedKey_MASK) {
			WIFI_DBG("\tPreSharedKey=%s\n", pObj->Security.PreSharedKey);
			if (wldm_AccessPoint_Security_PreSharedKey(CMD_SET_NVRAM, index,
				pObj->Security.PreSharedKey, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_KeyPassphrase_MASK) {
			WIFI_DBG("\tKeyPassphrase=%s\n", pObj->Security.KeyPassphrase);
			if (wldm_AccessPoint_Security_KeyPassphrase(CMD_SET_NVRAM, index,
				pObj->Security.KeyPassphrase, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_RekeyingInterval_MASK) {
			WIFI_DBG("\tRekeyingInterval=%d\n", pObj->Security.RekeyingInterval);
		}

		if (pObj->apply_map & AccessPoint_Security_RadiusServerIPAddr_MASK) {
			WIFI_DBG("\tRadiusServerIPAddr=%s\n", pObj->Security.RadiusServerIPAddr);
			len = strlen(pObj->Security.RadiusServerIPAddr) + 1;
			if (wldm_AccessPoint_Security_RadiusServerIPAddr(CMD_SET_NVRAM, index,
				pObj->Security.RadiusServerIPAddr, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_SecondaryRadiusServerIPAddr_MASK) {
			WIFI_DBG("\tSecondaryRadiusServerIPAddr=%s\n",
				pObj->Security.SecondaryRadiusServerIPAddr);
			len = strlen(pObj->Security.SecondaryRadiusServerIPAddr) + 1;
			if (wldm_AccessPoint_Security_SecondaryRadiusServerIPAddr(CMD_SET_NVRAM, index,
				pObj->Security.SecondaryRadiusServerIPAddr, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_RadiusServerPort_MASK) {
			WIFI_DBG("\tRadiusServerPort=%u\n", pObj->Security.RadiusServerPort);
			len = sizeof(pObj->Security.RadiusServerPort);
			if (wldm_AccessPoint_Security_RadiusServerPort(CMD_SET_NVRAM, index,
				&pObj->Security.RadiusServerPort, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_SecondaryRadiusServerPort_MASK) {
			WIFI_DBG("\tSecondaryRadiusServerPort=%d\n",
				pObj->Security.SecondaryRadiusServerPort);
			len = sizeof(pObj->Security.SecondaryRadiusServerPort);
			if (wldm_AccessPoint_Security_SecondaryRadiusServerPort(CMD_SET_NVRAM, index,
				&pObj->Security.SecondaryRadiusServerPort, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_RadiusSecret_MASK) {
			WIFI_DBG("\tRadiusSecret=%s\n", pObj->Security.RadiusSecret);
			len = strlen(pObj->Security.RadiusSecret) + 1;
			if (wldm_AccessPoint_Security_RadiusSecret(CMD_SET_NVRAM, index,
				pObj->Security.RadiusSecret, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_SecondaryRadiusSecret_MASK) {
			WIFI_DBG("\tSecondaryRadiusSecret=%s\n", pObj->Security.SecondaryRadiusSecret);
			len = strlen(pObj->Security.SecondaryRadiusSecret) + 1;
			if (wldm_AccessPoint_Security_SecondaryRadiusSecret(CMD_SET_NVRAM, index,
				pObj->Security.SecondaryRadiusSecret, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_MFPConfig_MASK) {
			WIFI_DBG("\tMFPConfig=%s\n", pObj->Security.MFPConfig);
			len = strlen(pObj->Security.MFPConfig) + 1;
			if (wldm_AccessPoint_Security_MFPConfig(CMD_SET_NVRAM, index,
				pObj->Security.MFPConfig, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
				ioctl_map |= AccessPoint_Security_MFPConfig_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObj->apply_map & AccessPoint_Security_Reset_MASK) {
			WIFI_DBG("\tReset=%d\n", (int)pObj->Security.Reset);
		}
	}

	if (pObjXRdk) {
		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_BasicAuthmode_MASK) {
			WIFI_DBG("\tBasicAuthentication=%s\n", pObjXRdk->Security.BasicAuthentication);
			len = strlen(pObjXRdk->Security.BasicAuthentication) + 1;
			if (wldm_AccessPoint_Basic_Authenticationmode(CMD_SET_NVRAM, index,
				pObjXRdk->Security.BasicAuthentication, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
				ioctl_map_XRdk |= X_RDK_AccessPoint_Security_BasicAuthmode_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_Encryption_MASK) {
			WIFI_DBG("\Encryption=%s\n", pObjXRdk->Security.Encryption);
			len = strlen(pObjXRdk->Security.Encryption) + 1;
			if (wldm_AccessPoint_Wpa_Encryptionmode(CMD_SET_NVRAM, index,
				pObjXRdk->Security.Encryption, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
				ioctl_map_XRdk |= X_RDK_AccessPoint_Security_Encryption_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_AuthMode_MASK) {
			WIFI_DBG("\tAuthMode=%d\n", pObjXRdk->Security.AuthMode);
			len = sizeof(pObjXRdk->Security.AuthMode);
			if (wldm_AccessPoint_Security_AuthMode(CMD_SET_NVRAM, index,
				(int *) &(pObjXRdk->Security.AuthMode), &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD;
				ioctl_map_XRdk |= X_RDK_AccessPoint_Security_AuthMode_MASK;
				ioctl_wl_down = 1;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_RadiusReAuthInterval_MASK) {
			WIFI_DBG("\tRadiusReAuthInterval=%u\n", pObjXRdk->Security.RadiusReAuthInterval);
			len = sizeof(pObjXRdk->Security.RadiusReAuthInterval);
			if (wldm_AccessPoint_Security_RadiusReAuthInterval(CMD_SET_NVRAM, index,
				&pObjXRdk->Security.RadiusReAuthInterval, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_RadiusOperatorName_MASK) {
			WIFI_DBG("\tRadiusOperatorName=%s\n", pObjXRdk->Security.RadiusOperatorName);
			len = strlen(pObjXRdk->Security.RadiusOperatorName) + 1;
			if (wldm_AccessPoint_Security_RadiusOperatorName(CMD_SET_NVRAM, index,
				pObjXRdk->Security.RadiusOperatorName, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_RadiusLocationData_MASK) {
			WIFI_DBG("\tRadiusLocationData=%s\n", pObjXRdk->Security.RadiusLocationData);
			len = strlen(pObjXRdk->Security.RadiusLocationData) + 1;
			if (wldm_AccessPoint_Security_RadiusLocationData(CMD_SET_NVRAM, index,
				pObjXRdk->Security.RadiusLocationData, &len, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_RadiusGreylist_MASK) {
			WIFI_DBG("\tRadiusGreylistEnable=%d\n", pObjXRdk->Security.RadiusGreylistEnable);
			if (wldm_AccessPoint_Security_RadiusGreylist(CMD_SET_NVRAM, index,
				&pObjXRdk->Security.RadiusGreylistEnable, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_RadiusDASPort_MASK) {
			WIFI_DBG("\tRadiusDASPort=%d\n", pObjXRdk->Security.RadiusDASPort);
			if (wldm_AccessPoint_Security_RadiusDASPort(CMD_SET_NVRAM, index,
				&pObjXRdk->Security.RadiusDASPort, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_RadiusDASClientIPAddr_MASK) {
			WIFI_DBG("\tRadiusDASClientIPAddr=%s\n", pObjXRdk->Security.RadiusDASClientIPAddr);
			if (wldm_AccessPoint_Security_RadiusDASClientIPAddr(CMD_SET_NVRAM, index,
				pObjXRdk->Security.RadiusDASClientIPAddr, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_RadiusDASSecret_MASK) {
			WIFI_DBG("\tRadiusDASSecret=%s\n", pObjXRdk->Security.RadiusDASSecret);
			if (wldm_AccessPoint_Security_RadiusDASSecret(CMD_SET_NVRAM, index,
				pObjXRdk->Security.RadiusDASSecret, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_WPAPairwiseRetries_MASK) {
			WIFI_DBG("\tWPAPairwiseRetries=%u\n", pObjXRdk->Security.WPAPairwiseRetries);
			if (wldm_AccessPoint_Security_WPAPairwiseRetries(CMD_SET_NVRAM, index,
				&pObjXRdk->Security.WPAPairwiseRetries, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_WPAPMKLifetime_MASK) {
			WIFI_DBG("\tWPAPMKLifetime=%u\n", pObjXRdk->Security.WPAPMKLifetime);
			if (wldm_AccessPoint_Security_WPAPMKLifetime(CMD_SET_NVRAM, index,
				&pObjXRdk->Security.WPAPMKLifetime, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}

		if (pObjXRdk->apply_map & X_RDK_AccessPoint_Security_WPA3TransitionDisable_MASK) {
			WIFI_DBG("\tWPA3TransitionDisable=%d\n", pObjXRdk->Security.WPA3TransitionDisable);
			if (wldm_AccessPoint_Security_WPA3TransitionDisable(CMD_SET_NVRAM, index,
				&pObjXRdk->Security.WPA3TransitionDisable, NULL, NULL, NULL) == 0) {
				action |= ACTION_APP_NVRAM_COMMIT;
			}
		}
	}

	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	/* If we need to change the radio on the fly, get the lock and then issue ioctls */
	if (deferred_acts & ACTION_DRV_WLCONF) {
		/* Wlconf is deferred and will be called later, no need to issue IOCTLs either */
	} else if (action & ACTION_DRV_WLCONF) {
		snprintf(buf, sizeof(buf), "wlconf %s up", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
		snprintf(buf, sizeof(buf), "wlconf %s start", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
	} else if (ioctl_map || ioctl_map_XRdk) {
		int lfd = -1;

		/* If we need to change the radio on the fly, get the lock and then issue ioctls */
		if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
			/* Aquire the driver lock locally */
			lfd = wldm_get_drvlock(radioIndex);
			if (lfd < 0) {
				WIFI_DBG("%s: No lock is aquired for radio %d!\n",
					__FUNCTION__, radioIndex);
			}
		}

		if (ioctl_wl_down) {
			/* Make WL down and apply the configs. */
			enable = 0;
			len = sizeof(enable);
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,
			(boolean *)&enable, &len, NULL, NULL);
		}

		/* Issue IOCTLs according to ioctl_map. Do wl down/up if necessary.
		*  Rearrange the order below according to wl driver's behavior.
		*/

		/* arranging the ioctl as encryption first and then authentcation */
		/* check if wl down/up should be done only once for the set of calls */

		if (ioctl_map_XRdk & X_RDK_AccessPoint_Security_BasicAuthmode_MASK) {
			len = strlen(pObjXRdk->Security.BasicAuthentication) + 1;
			wldm_AccessPoint_Basic_Authenticationmode(CMD_SET_IOCTL, index,
				pObjXRdk->Security.BasicAuthentication, &len, NULL, NULL);
		}

		if (ioctl_map & AccessPoint_Security_ModeEnabled_MASK) {
			len = strlen(pObj->Security.ModeEnabled) + 1;
			wldm_AccessPoint_Security_ModeEnabled(CMD_SET_IOCTL, index,
				pObj->Security.ModeEnabled, &len, NULL, NULL);
		}

		if (ioctl_map & AccessPoint_Security_MFPConfig_MASK) {
			len = strlen(pObj->Security.MFPConfig) + 1;
			wldm_AccessPoint_Security_MFPConfig(CMD_SET_IOCTL, index,
				pObj->Security.MFPConfig, &len, NULL, NULL);
		}

		if (ioctl_map_XRdk & X_RDK_AccessPoint_Security_Encryption_MASK) {
			len = strlen(pObjXRdk->Security.Encryption) + 1;
			wldm_AccessPoint_Wpa_Encryptionmode(CMD_SET_IOCTL, index,
				pObjXRdk->Security.Encryption, &len, NULL, NULL);
		}

		if (ioctl_map_XRdk & X_RDK_AccessPoint_Security_AuthMode_MASK) {
			len = sizeof(pObjXRdk->Security.AuthMode);
			wldm_AccessPoint_Security_AuthMode(CMD_SET_IOCTL, index,
				(int *) &(pObjXRdk->Security.AuthMode), &len, NULL, NULL);
		}
		if (ioctl_wl_down) {
			/* Make wl up after all the configs are done only if Radio is enabled. */
			len = sizeof(radio_isup);
			wldm_Radio_Enable(CMD_GET_NVRAM, radioIndex,
				(boolean *)&radio_isup, &len, NULL, NULL);
			if (radio_isup) {
				WIFI_DBG("%s: In nvram WiFi radio %s is set to be UP\n",
					__FUNCTION__, wl_ifname[radioIndex]);
				enable = 1;
				len = sizeof(enable);
				wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,
					(boolean *)&enable, &len, NULL, NULL);
			} else {
				WIFI_DBG("%s: In nvram WiFi radio %s is set to be Down\n",
					__FUNCTION__, wl_ifname[radioIndex]);
			}
		}

		if (lfd >= 0) {
			/* Release the driver lock */
			wldm_free_lock(lfd);
			lfd = -1;
		}
	}

	/* Restart security daemons. */
	if (action & ACTION_APP_HOSTAPD) {
		WIFI_DBG("%s: restart security daemons\n", __FUNCTION__);
		restart_wsec_daemons(radioIndex);
	}

	/* Free the temporary object after being applied */
	wldm_free_AccessPointSecurityObject(index);

	return ret;
}

int
wldm_apply_AccessPointSecurityObject(int apIndex)
{
	int ret;

	APINDEX_CHECK(apIndex, -1);
	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_AccessPointSecurityObject(apIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* AccessPoint_Security_RadiusSettings object */
/*
*  Free the contents and the object.
*  apIndex: AP Index.
*/
int
wldm_free_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(int apIndex)
{
	int index = apIndex;
	AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object *pObj;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(index, -1);

	pObj = WLDM_CNTXT_GET(objAccessPoint_Security_X_COMCAST_COM_RadiusSettings[index]);
	if (pObj == NULL)
		return 0;

	WLDM_CNTXT_SET(objAccessPoint_Security_X_COMCAST_COM_RadiusSettings[index], NULL);

	free(pObj);
	return 0;
}

/*
*  Return the AccessPointSecurityRadiusSettings_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index.
*/
AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object *
wldm_get_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(int apIndex)
{
	int index = apIndex;
	AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object *pObj;

	APINDEX_CHECK(index, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	pObj = WLDM_CNTXT_GET(objAccessPoint_Security_X_COMCAST_COM_RadiusSettings[index]);
	if (pObj == NULL) {
		pObj = calloc(1, sizeof(*pObj));
		if (pObj)
			WLDM_CNTXT_SET(objAccessPoint_Security_X_COMCAST_COM_RadiusSettings[index], pObj);
		else
			pthread_mutex_unlock(&wldm_cntxt_mutex);
	}

	return pObj;
}

static int
apply_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(int apIndex)
{
	int index = apIndex;
	int len = 0, ret = 0;
	int action = 0, acts_to_defer, deferred_acts = 0;
	int radioIndex = RADIO_INDEX(apIndex);
	int lfd = -1;
	AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object *pObj;
	WLDM_CNTXT_INIT(-1);

	pObj = WLDM_CNTXT_GET(objAccessPoint_Security_X_COMCAST_COM_RadiusSettings[index]);
	if (pObj == NULL) {
		return 0;
	}

	WIFI_DBG("%s: applying to %s: Device.WiFi.AccessPoint.%d.Security.X_COMCAST_COM_RadiusSettings\n",
		__FUNCTION__, wl_ifname[index], index + 1);

	WIFI_DBG("\tRadiusServerRetries=%d\n"
		"\tRadiusServerRequestTimeout=%d\n"
		"\tPMKLifetime=%d\n"
		"\tPMKCaching=%s\n"
		"\tPMKCacheInterval=%d\n"
		"\tMaxAuthenticationAttempts=%d\n"
		"\tBlacklistTableTimeout=%d\n"
		"\tIdentityRequestRetryInterval=%d\n"
		"\tQuietPeriodAfterFailedAuthentication=%d\n",
		pObj->RadiusSettings.RadiusServerRetries,
		pObj->RadiusSettings.RadiusServerRequestTimeout,
		pObj->RadiusSettings.PMKLifetime,
		pObj->RadiusSettings.PMKCaching ? "TRUE" : "FALSE",
		pObj->RadiusSettings.PMKCacheInterval,
		pObj->RadiusSettings.MaxAuthenticationAttempts,
		pObj->RadiusSettings.BlacklistTableTimeout,
		pObj->RadiusSettings.IdentityRequestRetryInterval,
		pObj->RadiusSettings.QuietPeriodAfterFailedAuthentication);

	if (WLDM_GET_LOCK(nvram_lockfd) < 0) {
		/* No lock is done, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	len = sizeof(pObj->RadiusSettings);
	if (wldm_AccessPoint_Security_X_COMCAST_COM_RadiusSettings(CMD_SET_NVRAM, index,
		&pObj->RadiusSettings, &len,
		NULL, NULL) == 0) {
		action |= (ACTION_APP_HOSTAPD | ACTION_APP_NVRAM_COMMIT);
	}

	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	/* Restart security daemons. */
	if (action & ACTION_APP_HOSTAPD) {
		WIFI_DBG("%s: restart security daemons\n", __FUNCTION__);
		restart_wsec_daemons(radioIndex);
	}

	/* Free the temporary object after being applied */
	wldm_free_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(index);

	return ret;
}

int
wldm_apply_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(int apIndex)
{
	int ret;

	APINDEX_CHECK(apIndex, -1);
	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(apIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* AccessPoint_WPS object */
/*
*  Free the contents and the object.
*  apIndex: AP Index.
*/
int
wldm_free_AccessPointWPSObject(int apIndex)
{
	int index = apIndex;
	AccessPoint_WPS_Object *pObj;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(index, -1);

	pObj = WLDM_CNTXT_GET(objAccessPoint_WPS[index]);
	if (pObj == NULL)
		return 0;

	WLDM_CNTXT_SET(objAccessPoint_WPS[index], NULL);

	/* Free the dynamically allocated RW contents(hexBinary, IPAddress, list, string) */
	if (pObj->Wps.ConfigMethodsEnabled) {
		free(pObj->Wps.ConfigMethodsEnabled);
		pObj->Wps.ConfigMethodsEnabled = NULL;
	}
	if (pObj->Wps.PIN) {
		free(pObj->Wps.PIN);
		pObj->Wps.PIN = NULL;
	}

	free(pObj);
	return 0;
}

/*
*  Return the AccessPointWPS_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
AccessPoint_WPS_Object *
wldm_get_AccessPointWPSObject(int apIndex, int checkMask)
{
	int index = apIndex;
	AccessPoint_WPS_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, AccessPoint_WPS_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objAccessPoint_WPS[index], checkMask);

	return pObj;
}

static int
apply_AccessPointWPSObject(int apIndex)
{
	int index = apIndex;
	int ret = 0, len = 0;
	int action = 0, acts_to_defer, deferred_acts = 0;
	int radioIndex = RADIO_INDEX(apIndex);
	int lfd = -1;
	AccessPoint_WPS_Object *pObj;
	WLDM_CNTXT_INIT(-1);

	pObj = WLDM_CNTXT_GET(objAccessPoint_WPS[index]);
	if (pObj == NULL) {
		return 0;
	}

	if (pObj->reject_map) {
		WIFI_ERR("%s: invalid parameters are present, reject map 0x%0x!\n",
			__FUNCTION__, pObj->reject_map);
		wldm_free_AccessPointWPSObject(index);
		return -2;
	}

	/* As of TR181 2.12, there are 3 RW parameters.
	*  Set the corresponding nvram variables, and determine how to restart.
	*/
	WIFI_DBG("%s: applying mapped 0x%0x params to %s: Device.WiFi.AccessPoint.%d.WPS.\n",
		__FUNCTION__, pObj->apply_map, wl_ifname[index], index + 1);

	if (WLDM_GET_LOCK(nvram_lockfd) < 0) {
		/* WPS is system-wize, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	if (pObj->apply_map & AccessPoint_WPS_Enable_MASK) {
		WIFI_DBG("\tEnable=%d\n", (int)pObj->Wps.Enable);
		len = sizeof(pObj->Wps.Enable);
		if (wldm_AccessPoint_WPS_Enable(CMD_SET_NVRAM, index,
			&pObj->Wps.Enable, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_WPS_ConfigMethodsEnabled_MASK) {
		WIFI_DBG("\tConfigMethodsEnabled=%s\n", pObj->Wps.ConfigMethodsEnabled);
		len = strlen(pObj->Wps.ConfigMethodsEnabled) + 1;
		if (wldm_AccessPoint_WPS_ConfigMethodsEnabled(CMD_SET_NVRAM, index,
			pObj->Wps.ConfigMethodsEnabled, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_WPS_PIN_MASK) {
		WIFI_DBG("\tPIN=%s\n", pObj->Wps.PIN);
		len = sizeof(pObj->Wps.PIN);
		if (wldm_AccessPoint_WPS_PIN(CMD_SET_NVRAM, index,
			pObj->Wps.PIN, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	/* Restart security daemon. */
	if (action & ACTION_APP_HOSTAPD) {
		/* Restart security daemons. */
		WIFI_DBG("%s: restart security daemons\n", __FUNCTION__);
		restart_wsec_daemons(radioIndex);
	}

	/* Free the temporary object after being applied */
	wldm_free_AccessPointWPSObject(index);

	return ret;
}

int
wldm_apply_AccessPointWPSObject(int apIndex)
{
	int ret;

	APINDEX_CHECK(apIndex, -1);
	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_AccessPointWPSObject(apIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* AccessPoint_AC object */
/*
*  Free the contents and the object.
*  apIndex: AP Index.
*/
int
wldm_free_AccessPointACObject(int apIndex, int acIndex)
{
	int index = (apIndex * AC_COUNT) + acIndex;
	AccessPoint_AC_Object *pObj;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(apIndex, -1);
	if (acIndex >= AC_COUNT || acIndex < 0) {
		WIFI_ERR("%s: invalid acIndex %d.\n", __FUNCTION__, acIndex);
		return -2;
	}

	pObj = WLDM_CNTXT_GET(objAccessPoint_AC[index]);
	if (pObj == NULL)
		return 0;

	WLDM_CNTXT_SET(objAccessPoint_AC[index], NULL);

	/* Free the dynamically allocated RW contents(hexBinary, IPAddress, list, string) */
	if (pObj->Ac.Alias) {
		free(pObj->Ac.Alias);
		pObj->Ac.Alias = NULL;
	}
	if (pObj->Ac.OutQLenHistogramIntervals) {
		free(pObj->Ac.OutQLenHistogramIntervals);
		pObj->Ac.OutQLenHistogramIntervals = NULL;
	}

	free(pObj);
	return 0;
}

/*
*  Return the AccessPointWPS_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
AccessPoint_AC_Object *
wldm_get_AccessPointACObject(int apIndex, int acIndex, int checkMask)
{
	int index = (apIndex * AC_COUNT) + acIndex;
	AccessPoint_AC_Object *pObj;

	APINDEX_CHECK(apIndex, NULL);
	if (acIndex >= AC_COUNT || acIndex < 0) {
		WIFI_DBG("%s: invalid acIndex %d.\n", __FUNCTION__, acIndex);
		return NULL;
	}

	OBJ_MASK_CHECK(checkMask, AccessPoint_AC_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objAccessPoint_AC[index], checkMask);

	return pObj;
}

static int
apply_AccessPointACObject(int apIndex, int acIndex)
{
	int index = (apIndex * AC_COUNT) + acIndex;
	int ret = 0;
	int action = 0, acts_to_defer, deferred_acts = 0;
	int ioctl_map = 0;	/* The map to config driver by issuing ioctl instead of wlconf. */
	int radioIndex = RADIO_INDEX(apIndex);
	int lfd = -1;
	AccessPoint_AC_Object *pObj;
	char nvram_name[128];
	char buf[256];
	char *osifname;
	WLDM_CNTXT_INIT(-1);

	pObj = WLDM_CNTXT_GET(objAccessPoint_AC[index]);
	if (pObj == NULL) {
		return 0;
	}

	if (pObj->reject_map) {
		WIFI_ERR("%s: invalid parameters are present, reject map 0x%0x!\n",
			__FUNCTION__, pObj->reject_map);
		wldm_free_AccessPointACObject(apIndex, acIndex);
		return -2;
	}

	/* As of TR181 2.12, there are 8 RW parameters.
	*  Set the corresponding nvram variables, and determine how to restart.
	*/
	osifname = wldm_get_radio_osifname(radioIndex);
	WIFI_DBG("%s: applying mapped 0x%0x params to %s: Device.WiFi.AccessPoint.%d.AC.%d.\n",
		__FUNCTION__, pObj->apply_map, wl_ifname[index], apIndex + 1, acIndex + 1);

	if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
		/* No lock is done, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	if (pObj->apply_map & AccessPoint_AC_Alias_MASK) {
		WIFI_DBG("\tAlias=%s\n", pObj->Ac.Alias);
		snprintf(nvram_name, sizeof(nvram_name), "%s_AccessPoint.AC.%d.Alias",
			wl_ifname[index], acIndex);
		if (wlcsm_nvram_set(nvram_name, pObj->Ac.Alias)) {
			WIFI_DBG("%s: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, nvram_name, pObj->Ac.Alias);
		}
		action |= ACTION_APP_NVRAM_COMMIT;
	}

	if (pObj->apply_map & AccessPoint_AC_AIFSN_MASK) {
		WIFI_DBG("\tAIFSN=%d\n", pObj->Ac.AIFSN);
	}

	if (pObj->apply_map & AccessPoint_AC_ECWMin_MASK) {
		WIFI_DBG("\tECWMin=%d\n", pObj->Ac.ECWMin);
	}

	if (pObj->apply_map & AccessPoint_AC_ECWMax_MASK) {
		WIFI_DBG("\tECWMax=%d\n", pObj->Ac.ECWMax);
	}

	if (pObj->apply_map & AccessPoint_AC_TxOpMax_MASK) {
		WIFI_DBG("\tTxOpMax=%d\n", pObj->Ac.TxOpMax);
	}

	if (pObj->apply_map & AccessPoint_AC_AckPolicy_MASK) {
		WIFI_DBG("\tAckPolicy=%d\n", pObj->Ac.AckPolicy);
	}

	if (pObj->apply_map & AccessPoint_AC_OutQLenHistogramIntervals_MASK) {
		WIFI_DBG("\tOutQLenHistogramIntervals=%s\n", pObj->Ac.OutQLenHistogramIntervals);
	}

	if (pObj->apply_map & AccessPoint_AC_OutQLenHistogramSampleInterval_MASK) {
		WIFI_DBG("\tOutQLenHistogramSampleInterval=%d\n",
			pObj->Ac.OutQLenHistogramSampleInterval);
	}

	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	/* Update the driver */
	if (deferred_acts & ACTION_DRV_WLCONF) {
		/* Wlconf is deferred and will be called later, no need to issue IOCTLs either */
	} else if (action & ACTION_DRV_WLCONF) {
		snprintf(buf, sizeof(buf), "wlconf %s up", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
		snprintf(buf, sizeof(buf), "wlconf %s start", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
	} else if (ioctl_map) {
		/* If we need to change the radio on the fly, get the lock and then issue ioctls */
		if (WLDM_TEST_LOCK(radioIndex) == FALSE) {
			/* Aquire the driver lock locally */
			lfd = wldm_get_drvlock(radioIndex);
			if (lfd < 0) {
				WIFI_DBG("%s: No lock is aquired for radio %d!\n",
					__FUNCTION__, radioIndex);
			}
		}

		/* Issue IOCTLs according to ioctl_map. Do wl down/up if necessary.
		*  Rearrange the order below according to wl driver's behavior.
		*/
		if (ioctl_map & AccessPoint_AC_AIFSN_MASK) {
		}

		if (ioctl_map & AccessPoint_AC_ECWMin_MASK) {
		}

		if (lfd >= 0) {
			/* Release the driver lock */
			wldm_free_lock(lfd);
		}
	}

	/* WMM is not handled by apps. */

	/* Free the temporary object after being applied */
	wldm_free_AccessPointACObject(apIndex, acIndex);

	return ret;
}

int
wldm_apply_AccessPointACObject(int apIndex, int acIndex)
{
	int ret;

	APINDEX_CHECK(apIndex, -1);
	if (acIndex >= AC_COUNT || acIndex < 0) {
		WIFI_ERR("%s: invalid acIndex %d.\n", __FUNCTION__, acIndex);
		return -1;
	}

	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_AccessPointACObject(apIndex, acIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* AccessPoint_Accounting object */
/*
*  Free the contents and the object.
*  apIndex: AP Index.
*/
int
wldm_free_AccessPointAccountingObject(int apIndex)
{
	int index = apIndex;
	AccessPoint_Accounting_Object *pObj;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(index, -1);

	pObj = WLDM_CNTXT_GET(objAccessPoint_Accounting[index]);
	if (pObj == NULL)
		return 0;

	WLDM_CNTXT_SET(objAccessPoint_Accounting[index], NULL);

	/* Free the dynamically allocated RW contents(hexBinary, IPAddress, list, string) */
	if (pObj->Accounting.ServerIPAddr) {
		free(pObj->Accounting.ServerIPAddr);
		pObj->Accounting.ServerIPAddr = NULL;
	}
	if (pObj->Accounting.SecondaryServerIPAddr) {
		free(pObj->Accounting.SecondaryServerIPAddr);
		pObj->Accounting.SecondaryServerIPAddr = NULL;
	}
	if (pObj->Accounting.Secret) {
		free(pObj->Accounting.Secret);
		pObj->Accounting.Secret = NULL;
	}
	if (pObj->Accounting.SecondarySecret) {
		free(pObj->Accounting.SecondarySecret);
		pObj->Accounting.SecondarySecret = NULL;
	}

	free(pObj);
	return 0;
}

/*
*  Return the AccessPointAccounting_Object pointer if previously allocated, otherwise alloc a new one.
*  apIndex: AP Index.
*  checkMask: assume in one TR181 SetParameterValues RPC, each parameter will only be issued once.
*/
AccessPoint_Accounting_Object *
wldm_get_AccessPointAccountingObject(int apIndex, int checkMask)
{
	int index = apIndex;
	AccessPoint_Accounting_Object *pObj;

	APINDEX_CHECK(index, NULL);

	OBJ_MASK_CHECK(checkMask, AccessPoint_Accounting_OBJ_MASKS, NULL);

	WLDM_CNTXT_INIT_AND_LOCK(NULL);
	GET_OBJECT(pObj, objAccessPoint_Accounting[index], checkMask);

	return pObj;
}

static int
apply_AccessPointAccountingObject(int apIndex)
{
	int index = apIndex;
	int ret = 0, len = 0;
	int action = 0, acts_to_defer, deferred_acts = 0;
	int radioIndex = RADIO_INDEX(apIndex);
	int lfd = -1;
	AccessPoint_Accounting_Object *pObj;
	WLDM_CNTXT_INIT(-1);

	APINDEX_CHECK(index, -1);

	pObj = WLDM_CNTXT_GET(objAccessPoint_Accounting[index]);
	if (pObj == NULL) {
		return 0;
	}

	if (pObj->reject_map) {
		WIFI_ERR("%s: invalid parameters are present, reject map 0x%0x!\n",
			__FUNCTION__, pObj->reject_map);
		wldm_free_AccessPointAccountingObject(index);
		return -2;
	}

	WIFI_DBG("%s: applying mapped 0x%0x params to %s: Device.WiFi.AccessPoint.%d.Accounting.\n",
		__FUNCTION__, pObj->apply_map, wl_ifname[index], index + 1);

	if (WLDM_GET_LOCK(nvram_lockfd) < 0) {
		/* Accounting is system-wize, aquire the nvram lock locally */
		lfd = wldm_get_nvramlock();
	}

	if (pObj->apply_map & AccessPoint_Accounting_Enable_MASK) {
		WIFI_DBG("\tEnable=%d\n", (int)pObj->Accounting.Enable);
		len = sizeof(pObj->Accounting.Enable);
		if (wldm_AccessPoint_Accounting_Enable(CMD_SET_NVRAM, index,
			&pObj->Accounting.Enable, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_Accounting_ServerIPAddr_MASK) {
		WIFI_DBG("\tServerIPAddr=%s\n", pObj->Accounting.ServerIPAddr);
		len = strlen(pObj->Accounting.ServerIPAddr) + 1;
		if (wldm_AccessPoint_Accounting_ServerIPAddr(CMD_SET_NVRAM, index,
			pObj->Accounting.ServerIPAddr, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_Accounting_ServerPort_MASK) {
		WIFI_DBG("\tServerPort=%d\n", pObj->Accounting.ServerPort);
		len = sizeof(pObj->Accounting.ServerPort);
		if (wldm_AccessPoint_Accounting_ServerPort(CMD_SET_NVRAM, index,
			&pObj->Accounting.ServerPort, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_Accounting_Secret_MASK) {
		WIFI_DBG("\tSecret=%s\n", pObj->Accounting.Secret);
		len = strlen(pObj->Accounting.Secret) + 1;
		if (wldm_AccessPoint_Accounting_Secret(CMD_SET_NVRAM, index,
			pObj->Accounting.Secret, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_Accounting_SecondaryServerIPAddr_MASK) {
		WIFI_DBG("\tSecondaryServerIPAddr=%s\n", pObj->Accounting.SecondaryServerIPAddr);
		len = strlen(pObj->Accounting.SecondaryServerIPAddr) + 1;
		if (wldm_AccessPoint_Accounting_SecondaryServerIPAddr(CMD_SET_NVRAM, index,
			pObj->Accounting.SecondaryServerIPAddr, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_Accounting_SecondaryServerPort_MASK) {
		WIFI_DBG("\tSecondaryServerPort=%d\n", pObj->Accounting.SecondaryServerPort);
		len = sizeof(pObj->Accounting.SecondaryServerPort);
		if (wldm_AccessPoint_Accounting_SecondaryServerPort(CMD_SET_NVRAM, index,
			&pObj->Accounting.SecondaryServerPort, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_Accounting_SecondarySecret_MASK) {
		WIFI_DBG("\tSecondarySecret=%s\n", pObj->Accounting.SecondarySecret);
		len = strlen(pObj->Accounting.SecondarySecret) + 1;
		if (wldm_AccessPoint_Accounting_SecondarySecret(CMD_SET_NVRAM, index,
			pObj->Accounting.SecondarySecret, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (pObj->apply_map & AccessPoint_Accounting_InterimInterval_MASK) {
		WIFI_DBG("\tInterimInterval=%d\n", pObj->Accounting.InterimInterval);
		len = sizeof(pObj->Accounting.InterimInterval);
		if (wldm_AccessPoint_Accounting_InterimInterval(CMD_SET_NVRAM, index,
			&pObj->Accounting.InterimInterval, &len, NULL, NULL) == 0) {
			action |= (ACTION_APP_NVRAM_COMMIT | ACTION_APP_HOSTAPD);
		}
	}

	if (lfd >= 0) {
		/* Release the nvram lock */
		wldm_free_lock(lfd);
		lfd = -1;
	}

	/* Filter the actions */
	acts_to_defer = WLDM_CNTXT_GET(actions_to_defer);
	if (action & acts_to_defer) {
		/* My action is to be deferred, mark it so it will be done later. */
		deferred_acts = WLDM_CNTXT_GET(deferred_actions[radioIndex]) |
			(action & acts_to_defer);
		WLDM_CNTXT_SET(deferred_actions[radioIndex], deferred_acts);
	}
	action &= ~deferred_acts;	/* Clean those actions. */

	if ((action & ACTION_APP_NVRAM_COMMIT) && (wlcsm_nvram_commit() != 0)) {
		WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	/* Restart security daemon. */
	if (action & ACTION_APP_HOSTAPD) {
		/* Restart security daemons. */
		WIFI_DBG("%s: restart security daemons\n", __FUNCTION__);
		restart_wsec_daemons(radioIndex);
	}

	/* Free the temporary object after being applied */
	wldm_free_AccessPointAccountingObject(index);

	return ret;
}

int
wldm_apply_AccessPointAccountingObject(int apIndex)
{
	int ret;

	APINDEX_CHECK(apIndex, -1);
	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = apply_AccessPointAccountingObject(apIndex);
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* Check if any previous setting needs to be applied. Return
*  -1: if any setting are rejected.
*  0: if no objects belonging to the given radioIndex is found.
*  > 0: number of objects that need to be applied(no rejection).
*/
static int
wldm_check_apply(int radioIndex)
{
	int ret = 0, apIndex, acIndex;
	Radio_Object *pObjRadio;
	SSID_Object *pObjSSID;
	AccessPoint_Object *pObjAccessPoint;
	AccessPoint_Security_Object *pObjAccessPoint_Security;
	AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object *pObjAccessPoint_Security_RadiusSettings;
	AccessPoint_WPS_Object *pObjAccessPoint_WPS;
	AccessPoint_AC_Object *pObjAccessPoint_AC;
	X_BROADCOM_COM_Radio_Object *pObjXBrcmRadio;
	X_RDK_AccessPoint_Security_Object *pObjX_RDK_AccessPoint_Security;
	X_RDK_Radio_Object *pObjXRDKRadio;
	X_RDK_AccessPoint_Object *pObjXRDKAccessPoint;
	X_LGI_Rates_Bitmap_Control_Object *pObjXLGIRatesBitmap;

	WLDM_CNTXT_INIT(-1);

	pObjRadio = WLDM_CNTXT_GET(objRadio[radioIndex]);
	if (pObjRadio) {
		if (pObjRadio->reject_map) {
			WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
				"Radio", radioIndex, pObjRadio->reject_map);
			return -1;
		}
		ret++;
		if (pObjRadio->apply_map) {
			/* TODO: Final check for OperatingFrequencyBand, Channel, etc.,
			*  parameters whose dependencies can not be determined in HAL set
			*  functions shall be done here.
			*/
		} else {
			WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
				__FUNCTION__, "Radio", radioIndex);
		}
	}

	pObjXBrcmRadio = WLDM_CNTXT_GET(objX_BROADCOM_COM_Radio[radioIndex]);
	if (pObjXBrcmRadio) {
		if (pObjXBrcmRadio->reject_map) {
			WIFI_DBG("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
				"X_BROADCOM_COM_Radio", radioIndex, pObjXBrcmRadio->reject_map);
		}
		ret++;
		if (pObjXBrcmRadio->apply_map) {
			/* TODO: Final check for AX Features.
			*  parameters whose dependencies can not be determined in HAL set
			*  functions shall be done here.
			*/
		} else {
			WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
				__FUNCTION__, "X_BROADCOM_COM_Radio", radioIndex);
		}
	}

	pObjXRDKRadio = WLDM_CNTXT_GET(objX_RDK_Radio[radioIndex]);
	if (pObjXRDKRadio) {
		if (pObjXRDKRadio->reject_map) {
			WIFI_DBG("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
				"X_RDK_Radio_Object", radioIndex, pObjXRDKRadio->reject_map);
		}
		ret++;
		if (pObjXRDKRadio->apply_map) {
			/* TODO: Final check for RDK radio Features.
			*  parameters whose dependencies can not be determined in HAL set
			*  functions shall be done here.
			*/
		} else {
			WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
				__FUNCTION__, "X_RDK_Radio_Object", radioIndex);
		}
	}

	for (apIndex = 0; apIndex < max_num_of_aps; apIndex++) {
		if (radioIndex != RADIO_INDEX(apIndex))
			continue;

		pObjSSID = WLDM_CNTXT_GET(objSSID[apIndex]);
		if (pObjSSID) {
			if (pObjSSID->reject_map) {
				WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
					"SSID", apIndex, pObjSSID->reject_map);
				return -1;
			}
			ret++;
			if (pObjSSID->apply_map == 0) {
				WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
					__FUNCTION__, "SSID", apIndex);
			}
		}

		pObjXLGIRatesBitmap = WLDM_CNTXT_GET(objX_LGI_RatesBitmap[apIndex]);
		if (pObjXLGIRatesBitmap) {
			if (pObjXLGIRatesBitmap->reject_map) {
				WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
					"pObjXLGIRatesBitmap", apIndex, pObjXLGIRatesBitmap->reject_map);
			}
			ret++;
			if (pObjXLGIRatesBitmap->apply_map == 0) {
				WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
					__FUNCTION__, "XLGIRatesBitmap", apIndex);
			}
		}

		pObjAccessPoint = WLDM_CNTXT_GET(objAccessPoint[apIndex]);
		if (pObjAccessPoint) {
			if (pObjAccessPoint->reject_map) {
				WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
					"AccessPoint", apIndex, pObjAccessPoint->reject_map);
				return -1;
			}
			ret++;
			if (pObjAccessPoint->apply_map == 0) {
				WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
					__FUNCTION__, "AccessPoint", apIndex);
			}
		}

		pObjXRDKAccessPoint = WLDM_CNTXT_GET(objX_RDK_AccessPoint[apIndex]);
		if (pObjXRDKAccessPoint) {
			if (pObjXRDKAccessPoint->reject_map) {
				WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
					"AccessPoint", apIndex, pObjXRDKAccessPoint->reject_map);
				return -1;
			}
			ret++;
			if (pObjXRDKAccessPoint->apply_map == 0) {
				WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
					__FUNCTION__, "AccessPoint", apIndex);
			}
		}

		pObjAccessPoint_Security = WLDM_CNTXT_GET(objAccessPoint_Security[apIndex]);
		if (pObjAccessPoint_Security) {
			if (pObjAccessPoint_Security->reject_map) {
				WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
					"AccessPointSecurity", apIndex,
					pObjAccessPoint_Security->reject_map);
				return -1;
			}
			ret++;
			if (pObjAccessPoint_Security->apply_map == 0) {
				WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
					__FUNCTION__, "AccessPointSecurity", apIndex);
			}
		}

		pObjX_RDK_AccessPoint_Security = WLDM_CNTXT_GET(objX_RDK_AccessPoint_Security[apIndex]);
		if (pObjX_RDK_AccessPoint_Security) {
			if (pObjX_RDK_AccessPoint_Security->reject_map) {
				WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
					"AccessPointSecurity", apIndex,
					pObjX_RDK_AccessPoint_Security->reject_map);
				return -1;
			}
			ret++;
			if (pObjX_RDK_AccessPoint_Security->apply_map == 0) {
				WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
				__FUNCTION__, "AccessPointSecurity", apIndex);
			}
		}

		pObjAccessPoint_Security_RadiusSettings = WLDM_CNTXT_GET(objAccessPoint_Security_X_COMCAST_COM_RadiusSettings[apIndex]);
		if (pObjAccessPoint_Security_RadiusSettings)
			ret++;

		pObjAccessPoint_WPS = WLDM_CNTXT_GET(objAccessPoint_WPS[apIndex]);
		if (pObjAccessPoint_WPS) {
			if (pObjAccessPoint_WPS->reject_map) {
				WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
					"AccessPointWPS", apIndex, pObjAccessPoint_WPS->reject_map);
				return -1;
			}
			ret++;
			if (pObjAccessPoint_WPS->apply_map == 0) {
				WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
					__FUNCTION__, "AccessPointWPS", apIndex);
			}
		}
		for (acIndex = 0; acIndex < AC_COUNT; acIndex++) {
			int index;

			index = (apIndex * AC_COUNT) + acIndex;
			pObjAccessPoint_AC = WLDM_CNTXT_GET(objAccessPoint_AC[index]);
			if (pObjAccessPoint_AC == NULL)
				continue;

			if (pObjAccessPoint_AC->reject_map) {
				WIFI_ERR("%s: %sObject[%d] reject_map=%x!\n", __FUNCTION__,
					"AccessPointAC", index, pObjAccessPoint_AC->reject_map);
				return -1;
			}
			ret++;
			if (pObjAccessPoint_AC->apply_map == 0) {
				WIFI_DBG("%s: %sObject[%d] allocated, apply_map not set!\n",
					__FUNCTION__, "AccessPointAC", index);
			}
		}
	}
	return ret;
}

static int
wldm_free_objects(int radioIndex)
{
	int ret = 0, apIndex, acIndex;

	wldm_free_RadioObject(radioIndex);
	for (apIndex = 0; apIndex < max_num_of_aps; apIndex++) {
		if (radioIndex != RADIO_INDEX(apIndex))
			continue;

		wldm_free_SSIDObject(apIndex);
		wldm_free_X_LGI_RatesControlObject(apIndex);
		wldm_free_AccessPointObject(apIndex);
		wldm_free_AccessPointSecurityObject(apIndex);
		wldm_free_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(apIndex);
		wldm_free_AccessPointWPSObject(apIndex);
		for (acIndex = 0; acIndex < AC_COUNT; acIndex++) {
			wldm_free_AccessPointACObject(apIndex, acIndex);
		}
	}
	return ret;
}

/* Free all allocated objects */
int
wldm_free_all(void)
{
	int radioIndex;

	for (radioIndex = 0; radioIndex < number_of_radios; radioIndex++) {
		wldm_free_objects(radioIndex);
	}
	return 0;
}

/* Check validity for all the objects */
static int
wldm_check_all(void)
{
	int count, radioIndex;

	/* In section "A.3.2.1 SetParameterValues" of TR069:
	*  On successful receipt of a SetParameterValues RPC, the CPE MUST apply the changes
	*  to all of the specified Parameters atomically. That is, either all of the value
	*  changes are applied together, or none of the changes are applied at all.
	*/
	for (count = radioIndex = 0; radioIndex < number_of_radios; radioIndex++) {
		if (wldm_check_apply(radioIndex) < 0) {
			/* Some setting shall be rejected */
			count++;
		}
	}
	if (count > 0) {
		return -1;
	}
	return 0;
}

static int
do_apply_all(void)
{
	int radioIndex, acts_to_defer, actions = 0, ret = 0;

	WLDM_CNTXT_INIT(-1);

	if (wldm_check_all() < 0) {
		wldm_free_all();
		WLDM_CNTXT_FREE();
		return -1;
	}

	/* Defer all actions */
	acts_to_defer = ACTION_SYS_RESTART | ACTION_DRV_WLCONF | ACTION_APP_NVRAM_COMMIT |
		ACTION_APP_ALL_DAEMONS | ACTION_APP_ACSD | ACTION_APP_HOSTAPD;
	for (radioIndex = 0; radioIndex < number_of_radios; radioIndex++) {
		ret = do_apply(radioIndex, acts_to_defer);
		if (ret == 0) {
			actions |= WLDM_CNTXT_GET(deferred_actions[radioIndex]);
		}
	}

	if (actions & acts_to_defer) {
		ret = schedule_deferred_actions(-1);
	} else if (wlcsm_nvram_commit_reqd() && wlcsm_nvram_commit() != 0) {
		WIFI_ERR("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
	}

	WLDM_CNTXT_FREE();
	WIFI_DBG("%s: done!\n", __FUNCTION__);
	return ret;
}

static int
do_apply(int radioIndex, int acts_to_defer)
{
	int ret, apIndex, acIndex, lfd, acts_to_defer_orig;
	WLDM_CNTXT_INIT(-1);

	ret = wldm_check_apply(radioIndex);
	if (ret < 0) {
		wldm_free_objects(radioIndex);
		return -1;
	} else if (ret == 0) {
		/* Check and commit if acts_to_defer is 0 for direct API call by upper layer. */
		if (acts_to_defer == 0 && wlcsm_nvram_commit_reqd() &&
			wlcsm_nvram_commit() != 0) {
			WIFI_ERR("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
		}
		return 0;
	}
	ret = 0;

	/* Acquire driver lock globally */
	lfd = wldm_get_drvlock(radioIndex);
	WLDM_GET_LOCK(driver_lockfd[radioIndex]) = lfd;

	acts_to_defer_orig = WLDM_CNTXT_GET(actions_to_defer);
	WIFI_DBG("%s: radio %d, actions_to_defer %x(%x)!\n", __FUNCTION__,
		radioIndex, acts_to_defer, acts_to_defer_orig);

	WLDM_CNTXT_SET(actions_to_defer, (acts_to_defer == 0) ? -1 : acts_to_defer);
	WLDM_CNTXT_SET(deferred_actions[radioIndex], 0);

	/* Apply shall be done from lower layer first, upper layers next */
	ret |= apply_RadioObject(radioIndex);
	for (apIndex = 0; apIndex < max_num_of_aps; apIndex++) {
		if (radioIndex != RADIO_INDEX(apIndex))
			continue;

		ret |= apply_AccessPointObject(apIndex);
		ret |= apply_AccessPointSecurityObject(apIndex);
		ret |= apply_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(apIndex);
		ret |= apply_AccessPointWPSObject(apIndex);
		for (acIndex = 0; acIndex < AC_COUNT; acIndex++) {
			ret |= apply_AccessPointACObject(apIndex, acIndex);
		}
		ret |= apply_AccessPointAccountingObject(apIndex);

		ret |= apply_SSIDObject(apIndex);
		ret |= apply_X_LGI_RatesControlObject(apIndex);
	}
	WLDM_CNTXT_SET(actions_to_defer, acts_to_defer_orig);

	if (lfd >= 0) {
		/* Release the global driver lock */
		WLDM_GET_LOCK(driver_lockfd[radioIndex]) = -1;
		wldm_free_lock(lfd);
	}

	return ret;
}

/* acts_to_defer:
 *    0, defer while applying settings, then do all actions for the given radio.
 *    otherwise, it contains ACTION_ bitmaps to defer.
 */
int
wldm_apply(int radioIndex, int acts_to_defer)
{
	int ret;

	RADIOINDEX_CHECK(radioIndex, -1);

	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = do_apply(radioIndex, acts_to_defer);
	pthread_mutex_unlock(&wldm_cntxt_mutex);

	if (ret == 0 && acts_to_defer == 0) {
		/* Only schedule if acts_to_defer is 0 for direct API call by upper layer. */
		ret = schedule_deferred_actions(radioIndex);
	}

	return ret;
}

/* Generic apply for all the objects */
int
wldm_apply_all(void)
{
	int ret;

	pthread_mutex_lock(&wldm_cntxt_mutex);
	ret = do_apply_all();
	pthread_mutex_unlock(&wldm_cntxt_mutex);
	return ret;
}

/* Return 0 if timer is scheduled, 1 if call do_deferred_actions() directly, error othewise.
 *    For a non-daemon app, sleep longer than WLDM_ACTION_WAIT_TIME_MS to wait for the timer.
 *
 * radioIndex:
 *    >= 0, check the deferred actions of the given radio only.
 *    < 0, check the deferred actions of all radios.
 */
static int
schedule_deferred_actions(int radioIndex)
{
	int i, tmp, radio_min, radio_max, schedule = FALSE;

	STOP_APPLY_TIMER();
	WLDM_CNTXT_INIT(-1);

	/* Do the deferred actions */
	if (radioIndex < 0) {
		/* All radios */
		radio_min = 0;
		radio_max = number_of_radios - 1;
	} else if (radioIndex < number_of_radios) {
		/* One particular radio */
		radio_min = radio_max = radioIndex;
	} else {
		WIFI_ERR("%s: radio index %d out of range!\n", __FUNCTION__, radioIndex);
		return -1;
	}

	pthread_mutex_lock(&radio_deferred_actions_mutex);
	for (i = radio_min; i <= radio_max; i++) {
		tmp = WLDM_CNTXT_GET(deferred_actions[i]);
		WLDM_CNTXT_SET(deferred_actions[i], 0);
		radio_deferred_actions[i] |= tmp;
		if (radio_deferred_actions[i]) {
			WIFI_DBG("%s: radio %d deferred actions 0x%x!\n", __FUNCTION__,
				i, radio_deferred_actions[i]);
			schedule = TRUE;
		}
	}
	pthread_mutex_unlock(&radio_deferred_actions_mutex);

	if (schedule == FALSE) {
		return -1;
	}

#ifdef WLDM_DEFER_TIMER
	if (defer_timer_start(WLDM_ACTION_WAIT_TIME_MS) >= 0) {
		WIFI_DBG("%s: timer scheduled!\n", __FUNCTION__);
		return 0;
	}

	/* Schedule timer failed, do it directly. */
	WIFI_ERR("%s: start defer timer failed!\n", __FUNCTION__);
#endif /* WLDM_DEFER_TIMER */
	do_deferred_actions(NULL);
	return 1;
}

/* For all threads of the process */
static int
do_deferred_actions(void *arg)
{
	int i, tmp, len, action, deferred_acts[MAX_WLAN_ADAPTER];
	char *osifname, iflist[STRING_LENGTH_128], buf[BUF_SIZE];
	pid_t pid;

	UNUSED_PARAMETER(arg);
	tmp = getpid();
	WIFI_DBG("\n%s(%d): %s...\n", __progname, tmp, __FUNCTION__);

	pthread_mutex_lock(&radio_deferred_actions_mutex);
	memcpy(deferred_acts, radio_deferred_actions, sizeof(deferred_acts));
	memset(radio_deferred_actions, 0, sizeof(radio_deferred_actions));
	pthread_mutex_unlock(&radio_deferred_actions_mutex);

	/* Get the deferred actions and the ifname list */
	memset(iflist, 0, sizeof(iflist));
	len = sizeof(iflist) - 1; /* Reserve ending '\0' for strncat */
	for (action = 0, i = 0; i < number_of_radios; i++) {
		tmp = deferred_acts[i];
		action |= tmp;
		if (tmp & ACTION_SYS_RESTART) {
			if (iflist[0] != '\0') {
				strncat(iflist, " ", len);
				len -= strlen(" ");
			}
			osifname = wldm_get_radio_osifname(i);
			strncat(iflist, osifname, len);
			len -= strlen(osifname);
#ifndef PHASE2_SEPARATE_RC
			wlcsm_mngr_restart(i, WLCSM_MNGR_RESTART_HTTPD, WLCSM_MNGR_RESTART_NOSAVEDM, 1);
#endif /* PHASE2_SEPARATE_RC */
		}
	}
	if (action) {
		WIFI_DBG("%s: action %x!\n", __FUNCTION__, action);
	}

	/* Commit nvram */
	if ((action & ACTION_APP_NVRAM_COMMIT) || wlcsm_nvram_commit_reqd()) {
		WIFI_DBG("%s: commit required, calling wlcsm_nvram_commit()...\n", __FUNCTION__);
		if (wlcsm_nvram_commit() != 0) {
			WIFI_DBG("%s: wlcsm_nvram_commit() failed\n", __FUNCTION__);
		}
	}

	/* Handle system restart */
	if (action & ACTION_SYS_RESTART) {
		/* Restart the radios listed in ifnames */
		WIFI_DBG("%s: restart if: %s!\n", __FUNCTION__, strlen(iflist) ? iflist : "all");
#ifdef PHASE2_SEPARATE_RC
		snprintf(buf, sizeof(buf), "wifi_setup.sh restart %s", iflist);
		WIFI_DBG("%s: system cmd [%s]\n", __FUNCTION__, buf);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
#endif /* PHASE2_SEPARATE_RC */
		goto done;
	}

	/* Reconfigure drivers by wlconf */
	for (i = 0; i < number_of_radios; i++) {
		tmp = deferred_acts[i];
		if ((tmp & ACTION_DRV_WLCONF) == 0)
			continue;

		osifname = wldm_get_radio_osifname(i);
		snprintf(buf, sizeof(buf), "wlconf %s up", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
		snprintf(buf, sizeof(buf), "wlconf %s start", osifname);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
	}

	/* Restart application daemons */
	if (action & ACTION_APP_ALL_DAEMONS) {
		/* Signal or restart all daemons */
		goto done;
	}

	/* Restart only the specified daemons */
	for (i = 0; i < number_of_radios; i++) {
		tmp = deferred_acts[i];
		if ((tmp & (ACTION_APP_HOSTAPD | ACTION_APP_HOSTAPD_STOP)) == 0)
			continue;

		if ((tmp & ACTION_APP_HOSTAPD) == 0) {
			WIFI_DBG("%s: stop security daemon for radio %d\n", __FUNCTION__, i);
			stop_wsec_daemons(i);
		} else {
			/* Restart security daemons. */
			WIFI_DBG("%s: restart security daemon for radio %d\n", __FUNCTION__, i);
			restart_wsec_daemons(i);
		}
	}

	if (action & ACTION_APP_ACSD) {
		pid = get_pid_by_name("acsd2");
		if (pid > 0) {
			dm_unregister_app_restart_info(pid);
			if (system("killall -q -9 acsd2") == -1) {
				WIFI_ERR("%s: Couldn't kill all acsd2 instanes\n", __FUNCTION__);
			}
		}
		if (system("acsd2&") == -1) {
			WIFI_ERR("%s: Couldn't start acsd2 in the background\n", __FUNCTION__);
		}
		WIFI_DBG("%s: acsd restarted!\n", __FUNCTION__);
	}

	wldm_hspot_restart_if_needed();

done:
	return 0;
}

/* END */
