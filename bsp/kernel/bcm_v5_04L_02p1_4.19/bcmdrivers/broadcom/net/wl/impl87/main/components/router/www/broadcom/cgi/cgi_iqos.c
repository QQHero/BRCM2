/*
 * Broadcom Home Gateway Reference Design
 * Web Page Configuration Support Routines
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
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: cgi_iqos.c 476403 2014-05-08 16:16:30Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>
#include <stdarg.h>
#include <string.h>
#ifndef BCA_HNDROUTER
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <unistd.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include <linux/list.h>
#ifndef BCA_HNDROUTER
#include <usr_ioctl.h>
#endif
#include <conf_app.h>
#include "json.h"
#include "cgi_iqos.h"
#include <ethernet.h>
#include <bcmnvram.h>
#include <shutils.h>
#include <confmtd_utils.h>
#ifdef BCA_HNDROUTER
#include <ioc_common.h>
#endif
#ifdef __CONFIG_TREND_IQOS__

#define MAC_OCTET_FMT "%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC_OCTET_EXPAND(o) \
	(uint8_t) o[0], (uint8_t) o[1], (uint8_t) o[2], (uint8_t) o[3], \
	(uint8_t) o[4], (uint8_t) o[5]

#define IPV4_OCTET_FMT "%u.%u.%u.%u"
#define IPV4_OCTET_EXPAND(o) (uint8_t) o[0], (uint8_t) o[1], (uint8_t) o[2], (uint8_t) o[3]

#define IPV6_OCTET_FMT "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X"
#define IPV6_OCTET_EXPAND(o) \
	(uint8_t) o[0], (uint8_t) o[1], (uint8_t) o[2], (uint8_t) o[3], \
	(uint8_t) o[4], (uint8_t) o[5], (uint8_t) o[6], (uint8_t) o[7], \
	(uint8_t) o[8], (uint8_t) o[9], (uint8_t) o[10], (uint8_t) o[11], \
	(uint8_t) o[12], (uint8_t) o[13], (uint8_t) o[14], (uint8_t) o[15]

#ifndef MIN
#define MIN(x, y) (((x) < (y) ? (x) : (y)))
#endif /* MIN */

#define IQOS_BUFF_SIZE 128
#define IQOS_MAX_DEVICES 64
#define IQOS_MAX_CAT_NODE 128
#define IQOS_MAX_URL_PARAMS 32
#define IQOS_MACSTR_LEN 24
#define IQOS_MAX_PRIORITY 7
#define IQOS_MAX_DEVICES_WITH_MACPRIORITY 32
#define BYTES_TO_BITS(x) ((x) * 8)
#define IQOS_MIN_REFRESH_RATE 5

#ifdef BCA_HNDROUTER
typedef struct app_ioc_entry devid_app_rate_e;
typedef struct udb_ioc_entry devid_user_e;
#define APP_DL_BYTE(app_lst, idx) (app_lst[idx].down_recent_accl)
#define APP_DL_PKT(app_lst, idx) (app_lst[idx].down_recent_accl_pkt)
#define APP_UL_BYTE(app_lst, idx) (app_lst[idx].up_recent_accl)
#define APP_UL_PKT(app_lst, idx) (app_lst[idx].up_recent_accl_pkt)
#define USR_TYPE_ID(idx) (usr_lst[idx].os.de.cat_id)
#define GET_FW_USERLIST(ul_p)	(get_fw_user_list(ul_p, NULL))
#else
#define APP_DL_BYTE(app_lst, idx) (app_lst[idx].down_accl_byte)
#define APP_DL_PKT(app_lst, idx) (app_lst[idx].down_accl_pkt)
#define APP_UL_BYTE(app_lst, idx) (app_lst[idx].up_accl_byte)
#define APP_UL_PKT(app_lst, idx) (app_lst[idx].up_accl_pkt)
#define USR_TYPE_ID(idx) (usr_lst[(idx)].os.detail.type_id)
#define GET_FW_USERLIST(ul_p)	(get_fw_user_list(ul_p))
#endif /* BCA_HNDROUTER */

/* Structure iqos_category_node required to represent category-wise info
	And category-wise Bandwidth calculation.
*/
struct iqos_category_node
{
	/* Category Identifier */
	int category_id;
	/* category is active (1->active and 0->inactive) */
	int active_flag;
	/* Current Up/Down Bytes per category */
	long unsigned int cat_total_new_bytes;
	/* Category Bandwidth */
	long unsigned int cat_bandwidth;
	/* Previous Up/Down Bytes per category */
	long unsigned int cat_total_prev_bytes;
	/* Current Up Bytes per category */
	long unsigned int cat_up_bytes;
	/* Current Down Bytes per category */
	long unsigned int cat_down_bytes;
	/* Accumulated Up Bytes per category */
	long unsigned int accu_cat_up_bytes;
	/* Accumulated Down Bytes per category */
	long unsigned int accu_cat_down_bytes;
};

struct iqos_device_info
{
	/* total category nodes per device */
	int cat_node_count;
	/* MAC address of an device */
	char mac_address[IQOS_BUFF_SIZE];
	/* device is active (1->active and 0->inactive) */
	int device_active_flag;
	/* device last state JSON string */
	char *device_previous_state;
	/* Current downloaded bytes for device */
	long unsigned int curr_download_bytes;
	/* Current uploaded bytes for device */
	long unsigned int curr_upload_bytes;
	/* Last downloaded bytes for device */
	long unsigned int last_download_bytes;
	/* Last uploaded bytes for device */
	long unsigned int last_upload_bytes;
	/* different category information */
	struct iqos_category_node *cat_nodes[IQOS_MAX_CAT_NODE];
};

struct iqos_app_node
{
	/* Category Identifier */
	int category_id;
	/* Application Identifier */
	int app_id;
	/* application state (0->InActive, 1->Active) */
	int visited_flag;
	/* Apps Previous download size in  bytes */
	long unsigned int app_prev_download_bytes;
	/* Apps Previous download and upload size in  bytes */
	long unsigned int app_prev_bytes;
	/* Time stamp when application was created */
	long unsigned int app_created_timestamp;
	/* Device MAC Address */
	char mac_address[IQOS_BUFF_SIZE];
	struct iqos_app_node *next;
};

typedef struct iqos_tokens {
	char *value;
	struct list_head list;
} iqos_tokens_t;

typedef enum {
	IQOS_UI_REQ_TYPE_DEVICE_LIST = 0,
	IQOS_UI_REQ_TYPE_DEVICE_SUMMARY,
	IQOS_UI_REQ_TYPE_GET_CONFIG,
	IQOS_UI_REQ_TYPE_SET_CONFIG,
	IQOS_UI_REQ_TYPE_GET_DEV_TYPE_PRIO,
	IQOS_UI_REQ_TYPE_SET_DEV_TYPE_PRIO,
	IQOS_UI_REQ_TYPE_GET_APP_CAT_PRIO,
	IQOS_UI_REQ_TYPE_SET_APP_CAT_PRIO,
	IQOS_UI_REQ_TYPE_GET_MAC_PRIO,
	IQOS_UI_REQ_TYPE_SET_MAC_PRIO,
	IQOS_UI_REQ_TYPE_SET_DEFAULT_PRIO,
	IQOS_UI_REQ_TYPE_SET_ALL_PRIO,
	IQOS_UI_REQ_TYPE_MAX
} iqos_ui_req_type_t;

#define iqos_Err_No_Input_Stream	-1
#define iqos_Err_Wrong_Request		-2
#define iqos_Err_Malformed_Request	-3

typedef struct iqos_ui_req_type_str {
	char *reqname;
	iqos_ui_req_type_t reqid;
} iqos_ui_req_type_str_t;

static iqos_ui_req_type_str_t giqos_req[] = {
	{"DeviceList",		IQOS_UI_REQ_TYPE_DEVICE_LIST},
	{"DeviceSummary",	IQOS_UI_REQ_TYPE_DEVICE_SUMMARY},
	{"GetConfig",		IQOS_UI_REQ_TYPE_GET_CONFIG},
	{"SetConfig",		IQOS_UI_REQ_TYPE_SET_CONFIG},
	{"GetDevTypePrio",	IQOS_UI_REQ_TYPE_GET_DEV_TYPE_PRIO},
	{"SetDevTypePrio",	IQOS_UI_REQ_TYPE_SET_DEV_TYPE_PRIO},
	{"GetAppCatPrio",	IQOS_UI_REQ_TYPE_GET_APP_CAT_PRIO},
	{"SetAppCatPrio",	IQOS_UI_REQ_TYPE_SET_APP_CAT_PRIO},
	{"GetMacPrio",		IQOS_UI_REQ_TYPE_GET_MAC_PRIO},
	{"SetMacPrio",		IQOS_UI_REQ_TYPE_SET_MAC_PRIO},
	{"SetDefaultPrio",	IQOS_UI_REQ_TYPE_SET_DEFAULT_PRIO},
	{"SetAllPrio",		IQOS_UI_REQ_TYPE_SET_ALL_PRIO}
};

#define IQOS_FIELDS_CONFIG_ENABLE_IQOS		0x0001
#define IQOS_FIELDS_CONFIG_BW_AUTO		0x0002
#define IQOS_FIELDS_CONFIG_DOWN_BW		0x0004
#define IQOS_FIELDS_CONFIG_UP_BW		0x0008

typedef struct iqos_ui_fields_req {
	char *name;
	unsigned int mask;
} iqos_ui_fields_req_t;

static iqos_ui_fields_req_t giqos_ui_fields[] = {
	{"EnableiQoS",		IQOS_FIELDS_CONFIG_ENABLE_IQOS},
	{"BWAuto",		IQOS_FIELDS_CONFIG_BW_AUTO},
	{"DownBW",		IQOS_FIELDS_CONFIG_DOWN_BW},
	{"UpBW",		IQOS_FIELDS_CONFIG_UP_BW},
	{"",			0}	/* Last entry must be empty string */
};

extern int errno;
static int iqos_process_req_devicelist(struct list_head *nvlist);
static int iqos_process_req_devicesummary(struct list_head *nvlist);
static int iqos_process_req_getconfig(struct list_head *nvlist);
static int iqos_process_req_setconfig(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary);
static int iqos_process_req_getdevtypeprio(struct list_head *nvlist);
static int iqos_process_req_setdevtypeprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary);
static int iqos_process_req_getappcatprio(struct list_head *nvlist);
static int iqos_process_req_setappcatprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary);
static int iqos_process_req_getmacprio(struct list_head *nvlist);
static int iqos_process_req_setmacprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary);
static const char *iqos_get_macprio(struct list_head *nvlist);
static const char *iqos_set_macprio(struct list_head *mac_list, FILE *stream, int len,
	const char *boundary);
static int iqos_get_mac_priority_ex(char *macaddr);
static int iqos_map_fields(struct list_head *fields_list, unsigned long long *flags);
static const char * iqos_get_config(unsigned int flags);
static int iqos_split_string(char *str, char *delim, struct list_head *tokenlist);
static int iqos_free_token_list(struct list_head *tokenlist);
static int iqos_print_tokens(struct list_head *tokenlist, char *prefix);
static int iqos_get_request_type(struct list_head *tokenlist);
static char *iqos_validate_url(char *url);
static char *iqos_get_value_from_list(struct list_head *tokenlist, char *name);
static const char* iqos_get_connected_devices(struct list_head *parameter_list);
static char* iqos_get_device_details(struct list_head *mac_addr_list,
	struct list_head *parameter_list);
static const char *iqos_get_devtypeprio(struct list_head *devtypes_list);
static const char *iqos_get_appcatprio(struct list_head *appcats_list);
static const char *iqos_create_status_response(char *req, int status);
static const char *iqos_set_config(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary);
static const char *iqos_set_devtypeprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary);
static const char * iqos_set_appcatprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary);
static int iqos_process_req_setdefaultprio(struct list_head *nvlist,
	FILE *stream, int len, const char *boundry);
static int iqos_process_req_setallprio(struct list_head *nvlist,
	FILE *stream, int len, const char *boundry);

static int iqos_verify_post_data(FILE *stream, int len, const char *boundary,
	struct json_object **jobj, char *expectedreq);
static void iqos_set_device_state(int device_index, const char *device_details);
static char* iqos_get_device_last_state(struct list_head *mac_addrs);
extern char* strncpy_n(char *destination, const char *source, size_t num);
static long long unsigned int iqos_remove_app_nodes();
static long long unsigned int iqos_get_total_nw_bandwidth(devid_user_e *usr_lst,
	devid_app_rate_e *app_lst);
static int
iqos_get_device_bandwidth_bytes(int device_index, long unsigned int *device_download_bytes,
	long unsigned int *device_upload_bytes);
static int iqos_search_in_list(struct list_head *tokenlist, const char *name);
static int iqos_get_devtype(char *macaddr);
static int iqos_file_lock(FILE *fp, int op);
static int iqos_file_restore(void);
static int iqos_load_default(void);
static void iqos_update_appcatprio(struct json_object *jobj);
static void  iqos_update_devtypeprio(struct json_object *jobj);
static void iqos_update_macprio(struct json_object *jobj);
static struct iqos_app_node *g_app_node_head = NULL;
static struct iqos_device_info *g_device_info_collection[IQOS_MAX_DEVICES] = {0};
static char* g_iqos_output_buffer = NULL;
/* Variable holds previous up+down Byte count on router */
static long long unsigned int	g_total_bandwidth_bytes = 0;

/* last Time-stamp called for method iqos_get_device_details which evaluate refresh rate */
static struct timespec g_timestamp_last_called = {0, 0};
static int g_iqos_restart_flag = IQOS_NOOP;
static time_t cur_time;
static int iqos_debug_flag = 0;
static int first_execution = 1;

#define IQOS_ERROR(fmt, arg...) \
		do { if (iqos_debug_flag & IQOS_DEBUG_ERROR) { \
			 cur_time = time(NULL); \
			 printf("%s [Error] %s: "fmt, asctime(localtime(&cur_time)), \
			 __FUNCTION__, ##arg); } } while (0)

#define IQOS_WARNING(fmt, arg...) \
		do { if (iqos_debug_flag & IQOS_DEBUG_WARNING) { \
			 cur_time = time(NULL); \
			 printf("%s [Warning] %s: "fmt, asctime(localtime(&cur_time)), \
			 __FUNCTION__, ##arg);} } while (0)

#define IQOS_INFO(fmt, arg...) \
		do { if (iqos_debug_flag & IQOS_DEBUG_INFO) { \
			 cur_time = time(NULL); \
			 printf("%s [Info] %s: "fmt, asctime(localtime(&cur_time)), \
			 __FUNCTION__, ##arg); } } while (0)

/*
Function iqos_add_device Will add new device found for category related calculations where
Parameters-----
ptr_mac_address: MAC Addr of Device [in]
device_index : device index to add [in]
*/
int
iqos_add_device(const char *ptr_mac_address, int device_index)
{
	struct iqos_device_info *ptrNode = NULL;
	ptrNode = (struct iqos_device_info *) calloc(sizeof(struct iqos_device_info), 1);
	if (!ptrNode) {
		IQOS_ERROR("Failed to allocate memory for New Device\n");
		return -1;
	}

	ptrNode->device_previous_state = NULL;
	ptrNode->last_download_bytes = 0;
	ptrNode->last_upload_bytes = 0;
	ptrNode->curr_download_bytes = 0;
	ptrNode->curr_upload_bytes = 0;
	ptrNode->device_active_flag = 1;
	strncpy_n(ptrNode->mac_address, ptr_mac_address, IQOS_BUFF_SIZE);
	g_device_info_collection[device_index] = ptrNode;
	g_device_info_collection[device_index]->cat_node_count = 0;

	IQOS_INFO("New Device %s Added at Index [%d]\n", ptr_mac_address, device_index);
	return 0;
}

/*
	Function iqos_update_device_info will add new category to category collection needed for
	category bandwidth calculations
	Parameters-----
	device_index : Existing Device index in collection [in]
	cat_current_up_bytes: up bytes of newly added catgory [in]
	cat_current_down_bytes: Down bytes of newly added catgory [in]
*/
static void
iqos_update_device_info(int device_index,
	const long unsigned int current_up_bytes,
	const long unsigned int current_down_bytes)
{
	struct iqos_device_info *device_info = NULL;
	device_info = g_device_info_collection[device_index];
	device_info->curr_download_bytes = current_down_bytes;
	device_info->curr_upload_bytes = current_up_bytes;
	device_info->device_active_flag = 1;
	IQOS_INFO("Device %s Updated at Index %d\n", device_info->mac_address, device_index);
}

/*
	Function iqos_add_category will add new category to category collection needed for category
	bandwidth calculations
	Parameters-----
	device_index : new/previous Device index in collection [in]
	category_id		: category identifier [in]
	cat_current_up_bytes: up bytes of newly added catgory [in]
	cat_current_down_bytes: Down bytes of newly added catgory [in]
*/
static int
iqos_add_category(const int device_index, const int category_id,
	const long unsigned int cat_current_up_bytes, long unsigned int cat_current_down_bytes)
{
	int index = 0;
	struct iqos_category_node *cat_node;
	struct iqos_device_info *device_info = NULL;

	device_info = g_device_info_collection[device_index];
	/* Find an empty place in the category array to add the new category */
	while ((index < IQOS_MAX_CAT_NODE) && (device_info->cat_nodes[index]))
		index++;

	if (index >= IQOS_MAX_CAT_NODE) {
		IQOS_ERROR("Category Buffer is Full. Cannot Add More Categories.\n");
		return -1;
	}

	cat_node = (struct iqos_category_node *)calloc(sizeof(struct iqos_category_node), 1);
	if (!cat_node) {
		IQOS_ERROR("Failed to allocate memory for New Category\n");
		return -1;
	}

	cat_node->category_id = category_id;
	cat_node->cat_total_prev_bytes = 0;
	cat_node->cat_up_bytes = cat_current_up_bytes;
	cat_node->cat_down_bytes = cat_current_down_bytes;
	cat_node->cat_total_new_bytes = cat_current_up_bytes + cat_current_down_bytes;
	cat_node->active_flag = 1;

	device_info->cat_nodes[index] = cat_node;
	device_info->cat_node_count += 1;

	IQOS_INFO("For Device %s New Category %d Added.\n", device_info->mac_address, category_id);
	return 0;
}

/*
	Function iqos_update_category_info will update/add  category to category
	collection which is needed for category	bandwidth calculations
	Parameters-----
	device_index    : new/previous Device index in collection [in]
	category_id		: category identifier [in]
	current_up_bytes: up bytes of newly added catgory [in]
	current_up_bytes: Down bytes of newly added catgory [in]
*/
void
iqos_update_category_info(int device_index, const int category_id,
	unsigned long int current_up_bytes, unsigned long int current_down_bytes)
{
	int cat_index = -1;
	int itr_cat;
	struct iqos_category_node *cat_node;
	struct iqos_device_info *device_info = g_device_info_collection[device_index];
	int add_status = 0;

	for (itr_cat = 0; itr_cat < IQOS_MAX_CAT_NODE; itr_cat++) {
		cat_node = device_info->cat_nodes[itr_cat];
		if (cat_node == NULL)
			continue;

		if (device_info->cat_nodes[itr_cat]->category_id == category_id) {
			cat_index = itr_cat;
			break;
		}
	}

	if (cat_index == -1) {
		add_status = iqos_add_category(device_index, category_id,
			current_up_bytes, current_down_bytes);
		if (add_status < 0)
			return;
	}
	else {

		device_info->cat_nodes[cat_index]->category_id = category_id;
		device_info->cat_nodes[cat_index]->cat_total_new_bytes +=
		current_up_bytes + current_down_bytes;

		device_info->cat_nodes[cat_index]->accu_cat_up_bytes +=	current_up_bytes;
		device_info->cat_nodes[cat_index]->accu_cat_down_bytes += current_down_bytes;

		device_info->cat_nodes[cat_index]->cat_up_bytes = current_up_bytes;
		device_info->cat_nodes[cat_index]->cat_down_bytes = current_down_bytes;
		device_info->cat_nodes[cat_index]->active_flag = 1;
	}
	IQOS_INFO("For Device %s Category [%d] Updated.\n", device_info->mac_address, category_id);
}

/*
	Function iqos_find_device will search for the device specified
	by Mac address in device collection and return accordingly
	Parameters-----
	device_index    : Device index found in collection [out]
	ptr_mac_address	: Mac Address of a device[in]
*/
static int
iqos_find_device(const char* ptr_mac_address, int* device_index)
{
	int itr_device = 0;
	for (itr_device = 0; itr_device < IQOS_MAX_DEVICES; itr_device++) {
		if (g_device_info_collection[itr_device] == NULL)
			continue;

		if (strcasecmp(g_device_info_collection[itr_device]->mac_address,
			ptr_mac_address) == 0) {
				*device_index = itr_device;
				IQOS_INFO("Device %s found At Index [%d].\n",
					ptr_mac_address, itr_device);
				return 1;
		}
	}
	IQOS_INFO("Device %s does not found.\n", ptr_mac_address);
	return 0;
}

/*
	Function iqos_set_category_cleanup will set adll the node
	in category collection as inactive
	Parameters-----
*/
void
iqos_set_category_cleanup()
{
	int itr_device;
	int itr_cat;
	struct iqos_category_node *cat_node;
	for (itr_device = 0; itr_device < IQOS_MAX_DEVICES; itr_device++) {
		if (g_device_info_collection[itr_device] == NULL)
			continue;

		for (itr_cat = 0; itr_cat <	IQOS_MAX_CAT_NODE; itr_cat++) {
			cat_node = g_device_info_collection[itr_device]->cat_nodes[itr_cat];
			if (cat_node == NULL)
				continue;

			cat_node->active_flag = 0;
			cat_node->cat_total_new_bytes = 0;
		}
	}
}

/*
	Function iqos_remove_category_nodes will delete all
	the inactive node in category collection.
Parameters-----
*/
void
iqos_remove_category_nodes()
{
	int itr_device;
	int itr_cat;
	struct iqos_category_node *cat_node;
	struct iqos_device_info *device_info = NULL;

	for (itr_device = 0; itr_device < IQOS_MAX_DEVICES; itr_device++) {
		if (g_device_info_collection[itr_device] == NULL)
			continue;

		device_info = g_device_info_collection[itr_device];
		for (itr_cat = 0; itr_cat < IQOS_MAX_CAT_NODE; itr_cat++) {
			cat_node = device_info->cat_nodes[itr_cat];
			if (cat_node == NULL)
				continue;

			if (cat_node->active_flag == 0) {
				IQOS_INFO("For Device %s, Category Removed [%d]\n",
					device_info->mac_address, cat_node->category_id);
				free(cat_node);
				device_info->cat_node_count -= 1;
				if (device_info->cat_node_count < 0)
					device_info->cat_node_count = 0;
				device_info->cat_nodes[itr_cat] = NULL;
			}
		}
	}
}

/*
	Function iqos_update_category_collection will update the values in in category collection.
	And calculate category bandwidth
	Parameters-----
		refresh_rate	: Time interval between function call (get_device_detail) [in]
*/
void
iqos_update_category_collection(int device_index, const int refresh_rate)
{
	int itr_cat;
	struct iqos_category_node *cat_node;
	struct iqos_device_info *device_info = g_device_info_collection[device_index];

	for (itr_cat = 0; itr_cat < IQOS_MAX_CAT_NODE; itr_cat++) {

		cat_node = device_info->cat_nodes[itr_cat];
		if (cat_node == NULL)
			continue;

		if (cat_node->cat_total_new_bytes >= cat_node->cat_total_prev_bytes) {
			cat_node->cat_bandwidth = (cat_node->cat_total_new_bytes -
				cat_node->cat_total_prev_bytes) / refresh_rate;
		} else {
			cat_node->cat_bandwidth = cat_node->cat_total_new_bytes /
				refresh_rate;
		}
		cat_node->cat_total_prev_bytes = cat_node->cat_total_new_bytes;
	}
	IQOS_INFO("For Device %s, Category Collection Updated.\n", device_info->mac_address);
}

/*
Function iqos_append_app_node will append the new iqos_app_node in application node collection.
Parameters-----
	ptr_mac_addr : Device MAC Address [in]
	category_id	   : Category Identifier [in]
	app_id	   : Application Idetifier [in]
	app_prv_bw : Application previous bandwidth [in]
	app_prev_download_bytes : Application previous download bytes [in]
	app_create_time : Application Create Time [in]
*/
static int
iqos_append_app_node(const char *ptr_mac_addr, const int category_id, const int app_id,
	const long unsigned int app_prv_bw, long unsigned int app_prv_download_bytes,
	const long unsigned int app_create_time)
{
	struct iqos_app_node *new_node, *right;
	new_node = (struct iqos_app_node *)calloc(sizeof(struct iqos_app_node), 1);
	if (!new_node) {
		IQOS_ERROR("Failed to allocate memory for New Application Node.\n");
		return -1;
	}

	new_node->app_id = app_id;
	new_node->category_id = category_id;
	new_node->visited_flag = 1;

	strncpy_n(new_node->mac_address, ptr_mac_addr, IQOS_BUFF_SIZE);

	new_node->app_prev_bytes = app_prv_bw;
	new_node->app_prev_download_bytes = app_prv_download_bytes;
	new_node->app_created_timestamp = app_create_time;
	new_node->next = NULL;

	right = (struct iqos_app_node *)g_app_node_head;
	while (right->next)
		right = right->next;
	right->next = new_node;

	IQOS_INFO("For Device %s, New Application Node with id [%d] Added.\n",
		ptr_mac_addr, app_id);
	return 0;
}

/* Function iqos_add_app_node will add the new iqos_app_node in application node collection.
Parameters-----
	ptr_mac_addr : Device MAC Address [in]
	category_id	   : Category Identifier [in]
	app_id	   : Application Idetifier [in]
	app_prv_bw : Application previous bandwidth [in]
	app_prev_download_bytes : Application previous download bytes [in]
	app_create_time : Application Create Time [in]
*/
static int
iqos_add_app_node(const char *ptr_mac_addr, const int category_id, const int app_id,
	const long unsigned int app_prv_bw, long unsigned int app_prv_download_bytes,
	long unsigned int app_create_time)
{
	struct iqos_app_node *new_node;
	new_node = (struct iqos_app_node *)calloc(sizeof(struct iqos_app_node), 1);
	if (!new_node) {
		IQOS_ERROR("Failed to allocate memory for New Application Node.\n");
		return -1;
	}

	new_node->app_id = app_id;
	new_node->category_id = category_id;
	new_node->visited_flag = 1;
	strncpy_n(new_node->mac_address, ptr_mac_addr, IQOS_BUFF_SIZE);
	new_node->app_prev_bytes = app_prv_bw;
	new_node->app_prev_download_bytes = app_prv_download_bytes;
	new_node->app_created_timestamp = app_create_time;
	if (g_app_node_head == NULL) {
		g_app_node_head = new_node;
		g_app_node_head->next = NULL;
	}
	else {
		new_node->next = g_app_node_head;
		g_app_node_head = new_node;
	}

	IQOS_INFO("For Device %s, New Application Node with id [%d] Added.\n",
		ptr_mac_addr, app_id);
	return 0;
}

/* Function iqos_find_app_node will find the iqos_app_node in application node collection.
	If found then update values or add new one
Parameters-----
	ptr_mac_addr : Device MAC Address [in]
	category_id	   : Category Identifier [in]
	app_id	   : Application Idetifier [in]
	curr_app_bytes : Application current accumulative size of bytes [in]
	curr_app_down_bytes : Application current accumulative size of download bytes [in]
	last_used_time : Application Last used Time stamp [out]
*/
static long unsigned int
iqos_find_app_node(char *ptr_mac_addr, int category_id, int app_id,
	long unsigned int curr_app_bytes, long unsigned int curr_app_down_bytes,
	long unsigned int *last_used_time)
{
	struct iqos_app_node *iterator_node = g_app_node_head;
	long unsigned int iPrevVal = 0;
	while (iterator_node) {
		if (iterator_node->category_id == category_id && iterator_node->app_id == app_id &&
			strcasecmp(iterator_node->mac_address, ptr_mac_addr) == 0) {
			iterator_node->visited_flag = 1;
			iPrevVal = iterator_node->app_prev_download_bytes;
			iterator_node->app_prev_download_bytes = curr_app_down_bytes;
			iterator_node->app_prev_bytes = curr_app_bytes;
			*last_used_time = *last_used_time - iterator_node->app_created_timestamp;
			return iPrevVal;
		}
		iterator_node = iterator_node->next;
	}

	if (g_app_node_head != NULL) {
		iqos_append_app_node(ptr_mac_addr, category_id,
			app_id, curr_app_bytes, curr_app_down_bytes, *last_used_time);
	} else {
		iqos_add_app_node(ptr_mac_addr, category_id, app_id, curr_app_bytes,
			curr_app_down_bytes, *last_used_time);
	}

	*last_used_time = 0;
	return iPrevVal;
}

/* Function iqos_remove_app_nodes will Remove inactive Application node from App node collection
	Parameters-----
*/
static long long unsigned int
iqos_remove_app_nodes()
{
	struct iqos_app_node *iterator_node, *prev;
	long long unsigned int dead_bytes = 0;
	iterator_node = g_app_node_head;
	prev = g_app_node_head;
	if (!iterator_node)
		return 0;
	while (iterator_node) {
		if (iterator_node->visited_flag == 0) {
			if (iterator_node == g_app_node_head) {
				g_app_node_head = iterator_node->next;
				dead_bytes += iterator_node->app_prev_bytes;
				IQOS_INFO("For Device %s, Application Node with id [%d] Deleted.\n",
					iterator_node->mac_address, iterator_node->app_id);
				free(iterator_node);
				iterator_node = g_app_node_head;
			}
			else {
				prev->next = iterator_node->next;
				dead_bytes += iterator_node->app_prev_bytes;
				IQOS_INFO("For Device %s, Application Node with id [%d] Deleted.\n",
					iterator_node->mac_address, iterator_node->app_id);
				free(iterator_node);
				iterator_node = prev->next;
			}

		} else {
			prev = iterator_node;
			iterator_node = iterator_node->next;
		}
	}

	return dead_bytes;
}

/* Function iqos_get_device_bandwidth_bytes will update the device up/down byte value requiered for
	bandwidth calculation
	Parameters-----
	ptr_mac_addr : Device MAC Address [in]
	prev_down_bytes   : previous Download bytes for device[out]
	prev_up_bytes	   : previous up bytes for device [out]
*/
static int
iqos_get_device_bandwidth_bytes(int device_index, long unsigned int *device_download_bytes,
	long unsigned int *device_upload_bytes)
{
	struct iqos_device_info *device_info = g_device_info_collection[device_index];

	if (device_info) {
		if (device_info->curr_download_bytes >= device_info->last_download_bytes) {
			*device_download_bytes = device_info->curr_download_bytes -
				device_info->last_download_bytes;
		} else {
			*device_download_bytes = device_info->curr_download_bytes;
		}

		device_info->last_download_bytes = device_info->curr_download_bytes;

		if (device_info->curr_upload_bytes >= device_info->last_upload_bytes) {
			*device_upload_bytes = device_info->curr_upload_bytes -
				device_info->last_upload_bytes;
		} else {
			*device_upload_bytes = device_info->curr_upload_bytes;
		}

		device_info->last_upload_bytes = device_info->curr_upload_bytes;
	}

	return 0;
}

/* Function iqos_set_node_not_visited will mark all Application node as inactive
	in an App node collection
*/
int iqos_set_node_not_visited()
{
	struct iqos_app_node *iterator_node = g_app_node_head;
	if (iterator_node == NULL) {
		return 0;
	}
	while (iterator_node) {
		iterator_node->visited_flag = 0;
		iterator_node = iterator_node->next;
	}
	return 0;
}

/* Function iqos_lu_to_str will convert long unsigned int value to string
	Parameters-----
	value	: value to convert [in]
	buffer: output string buffer [out]
*/
char*  iqos_lu_to_str(const long unsigned int value, char *buffer, int buf_size)
{
	snprintf(buffer, buf_size, "%lu", value);
	return buffer;
}

/* Function iqos_llu_to_str will convert long long unsigned int
	value to string
	Parameters-----
	value	: value to convert [in]
	buffer: output string buffer [out]
	buf_size : buffer size
*/
char*  iqos_llu_to_str(const long long unsigned int value, char *buffer, int buf_size)
{
	snprintf(buffer, buf_size, "%llu", value);
	return buffer;
}

void
iqos_confirm_app_node(const char *mac_addr, const int cat_id, const int app_id)
{
	struct iqos_app_node *iterator_node = g_app_node_head;
	if (iterator_node == NULL)
		return;

	while (iterator_node) {
		if (iterator_node->category_id == cat_id && iterator_node->app_id == app_id &&
			strcasecmp(iterator_node->mac_address, mac_addr) == 0) {
				iterator_node->visited_flag = 1;
				break;
			}
		iterator_node = iterator_node->next;
	}
}

static void
iqos_remove_dead_devices()
{
	int itr_device = 0;
	struct iqos_device_info *device_info = NULL;
	for (itr_device = 0; itr_device < IQOS_MAX_DEVICES; itr_device++) {
		device_info = g_device_info_collection[itr_device];
		if (device_info == NULL)
			continue;

		if (device_info->device_active_flag == 0) {
			if (device_info->device_previous_state) {
				free(device_info->device_previous_state);
				device_info->device_previous_state = NULL;
			}
			IQOS_INFO("Device %s Removed\n", device_info->mac_address);
			free(device_info);
			g_device_info_collection[itr_device] = NULL;
		}
	}
}

static void
iqos_set_device_cleanup_flag()
{
	int itr_device = 0;
	for (itr_device = 0; itr_device < IQOS_MAX_DEVICES; itr_device++) {
		if (g_device_info_collection[itr_device] == NULL)
			continue;
		g_device_info_collection[itr_device]->device_active_flag = 0;
	}
}

static void
iqos_validate_devices(devid_user_e *usr_lst)
{
	int itr_user_rec = 0;
	int device_index = -1;
	char mac_address[IQOS_BUFF_SIZE];
	struct iqos_device_info *device_info = NULL;

	iqos_set_device_cleanup_flag();
	for (itr_user_rec = 0; itr_user_rec < DEVID_MAX_USER; itr_user_rec++) {
		if (usr_lst[itr_user_rec].available <= 0) {
			break;
		}
		snprintf(mac_address, sizeof(mac_address), MAC_OCTET_FMT,
			MAC_OCTET_EXPAND(usr_lst[itr_user_rec].mac));

		if (iqos_find_device(mac_address, &device_index)) {
			device_info = g_device_info_collection[device_index];
			if (device_info) {
				device_info->device_active_flag = 1;
			}
		}
	}
}

/* Function iqos_get_total_nw_bandwidth gives total traffic on the router in bytes
	Parameters-----
	devid_app_rate_e *app_lst : [in]
	devid_app_rate_e *app_lst : [in]
	@return : bandwidth changed by
*/
static long long unsigned int
iqos_get_total_nw_bandwidth(devid_user_e *usr_lst, devid_app_rate_e *app_lst)
{
	int itr_app_rec = 0, itr_user_rec = 0;
	char mac_address[IQOS_BUFF_SIZE] = {0};
	long long unsigned int total_bw_bytes = 0, dead_bytes = 0;
	long long unsigned int bw_changed_by = 0;

	iqos_set_node_not_visited();
	for (itr_user_rec = 0; itr_user_rec < DEVID_MAX_USER; itr_user_rec++) {
		if (usr_lst[itr_user_rec].available <= 0) {
			break;
		}
		snprintf(mac_address, sizeof(mac_address), MAC_OCTET_FMT,
			MAC_OCTET_EXPAND(usr_lst[itr_user_rec].mac));
		for (itr_app_rec = 0; itr_app_rec < DEVID_APP_RATE_TABLE_POOL_SIZE; itr_app_rec++) {
			if (app_lst[itr_app_rec].available <= 0)
				break;
			if (app_lst[itr_app_rec].uid == usr_lst[itr_user_rec].uid) {
				total_bw_bytes += APP_DL_BYTE(app_lst, itr_app_rec);
				total_bw_bytes += APP_UL_BYTE(app_lst, itr_app_rec);
				iqos_confirm_app_node(mac_address, app_lst[itr_app_rec].cat_id,
					app_lst[itr_app_rec].app_id);
			}
		}
	}
	dead_bytes = iqos_remove_app_nodes();

	/* removing dead bytes from previous bandwidth value */
	if (dead_bytes > g_total_bandwidth_bytes)
		g_total_bandwidth_bytes = dead_bytes;
	else
		g_total_bandwidth_bytes -= dead_bytes;

	/* calculating current bandwidth bytes */
	if (total_bw_bytes >=  g_total_bandwidth_bytes)
		bw_changed_by = total_bw_bytes - g_total_bandwidth_bytes;
	else
		g_total_bandwidth_bytes = total_bw_bytes;

	/* setting new bandwidth bytes to old one */
	g_total_bandwidth_bytes = total_bw_bytes;

	/* return bandwidth change */
	return bw_changed_by;
}

/* Function iqos_inline_url_unescape: unescape a URL. Change URL escape sequences like %3A
	to ':'. Can be extended to incorporate further sequences on need basis.
	Important: This function changes the original url buffer that is passed to it

	Parameters-----
	url	: url to change
*/

static int
iqos_inline_url_unescape(char *url)
{
	char *p = url;
	int i = 0;

	if (url == NULL) {
	return -1;
	}

	while (*p != '\0') {
		if ((*p == '%') && (*(p + 1) == '3') && (*(p + 2) == 'A')) {
			url[i] = ':';
			p += 3;
		} else if ((*p == '%') && (*(p + 1) == '2') && (*(p + 2) == '0')) {
			url[i] = ' ';
			p += 3;
	} else {
			url[i] = *p;
			p++;
		}
		i++;
	}
	url[i] = '\0';
	return 0;
}

/* Function iqos_get_connected_devices will gives list of connected Devices to router in JSON format
	Parameters-----
		parameter_list -: parameters to include in final JSON object.
*/
const char* iqos_get_connected_devices(struct list_head *parameter_list)
{
	unsigned int itr_user_rec;
	devid_user_e *usr_lst = NULL;
	const char* data = NULL;
	const char* ptr_device_list = NULL;
	dev_os_t *dev_os;
	char buffer[IQOS_BUFF_SIZE] = {'\0'};
	char macaddr[IQOS_MACSTR_LEN];
	json_object	*object;
	json_object	*main_object;
	json_object	*array_conn_device_list;
	int ret = 0;
	LIST_HEAD(dev_os_head);

	ret = GET_FW_USERLIST(&usr_lst);
	if (ret < 0) {
		if (usr_lst) {
			free(usr_lst);
			usr_lst = NULL;
		}
		IQOS_ERROR("Unable to populate User List.\n");
		return NULL;
	}

	init_dev_os(&dev_os_head);
	if (usr_lst) {
		main_object = json_object_new_object();
		json_object_object_add(main_object, "Req", json_object_new_string("DeviceList"));
		array_conn_device_list = json_object_new_array();
		for (itr_user_rec = 0; itr_user_rec <= DEVID_MAX_USER; itr_user_rec++) {
			if (usr_lst[itr_user_rec].available <= 0) {
				break;
			}

			if (strlen(usr_lst[itr_user_rec].host_name) == 0)
				usr_lst[itr_user_rec].host_name[0] = '\0';
			object = json_object_new_object();

			if (iqos_search_in_list(parameter_list, "Name")) {
			json_object_object_add(object, "Name",
				json_object_new_string(usr_lst[itr_user_rec].host_name));
			}
#ifdef BCA_HNDROUTER
			dev_os = search_dev_os(&dev_os_head,
					usr_lst[itr_user_rec].os.de.vendor_id,
					usr_lst[itr_user_rec].os.de.name_id,
					usr_lst[itr_user_rec].os.de.class_id,
					usr_lst[itr_user_rec].os.de.cat_id,
					usr_lst[itr_user_rec].os.de.dev_id,
					usr_lst[itr_user_rec].os.de.family_id);
#else /* BCA_HNDROUTER */
			dev_os = search_dev_os(&dev_os_head,
				usr_lst[itr_user_rec].os.detail.vendor_id,
				usr_lst[itr_user_rec].os.detail.os_id,
				usr_lst[itr_user_rec].os.detail.class_id,
				usr_lst[itr_user_rec].os.detail.type_id,
				usr_lst[itr_user_rec].os.detail.dev_id,
				usr_lst[itr_user_rec].os.detail.family_id);
#endif /* !BCA_HNDROUTER */

			if (dev_os) {
				if (iqos_search_in_list(parameter_list, "DevType")) {
					json_object_object_add(object, "DevType",
						json_object_new_string(dev_os->type_name));
				}

				if (iqos_search_in_list(parameter_list, "Name")) {
					json_object_object_add(object, "DevName",
						json_object_new_string(dev_os->dev_name));
				}
			}

			if (iqos_search_in_list(parameter_list, "DevTypeInt")) {
				json_object_object_add(object, "DevTypeInt",
					json_object_new_int(
					USR_TYPE_ID(itr_user_rec)));
			}

			snprintf(macaddr, sizeof(macaddr), MAC_OCTET_FMT,
				MAC_OCTET_EXPAND(usr_lst[itr_user_rec].mac));
			if (iqos_search_in_list(parameter_list, "MacAddr")) {
				json_object_object_add(object, "MacAddr",
					json_object_new_string(macaddr));
			}

			if (iqos_search_in_list(parameter_list, "Priority")) {
				json_object_object_add(object, "Priority",
					json_object_new_string(iqos_lu_to_str(
						iqos_get_mac_priority_ex(macaddr),
						buffer, sizeof(buffer))));
			}

			snprintf(buffer, sizeof(buffer),
				IPV4_OCTET_FMT, IPV4_OCTET_EXPAND(usr_lst[itr_user_rec].ipv4));
			if (iqos_search_in_list(parameter_list, "IPAddr")) {
				json_object_object_add(object, "IPAddr",
					json_object_new_string(buffer));
			}

			if (dev_os) {
				if (iqos_search_in_list(parameter_list,
					"OS") != 0) {
					json_object_object_add(object, "OS",
						json_object_new_string(dev_os->os_name));
				}
			}

			if (iqos_search_in_list(parameter_list, "LastUsed")) {
				json_object_object_add(object, "LastUsed",
					json_object_new_string(iqos_lu_to_str(
					usr_lst[itr_user_rec].ts, buffer, sizeof(buffer))));
				}

			json_object_array_add(array_conn_device_list, object);
		}
		json_object_object_add(main_object, "Device", array_conn_device_list);
		ptr_device_list = json_object_to_json_string(main_object);
		data = strdup(ptr_device_list);
		json_object_put(main_object);
	}
	if (usr_lst) {
		free(usr_lst);
	}
	free_dev_os(&dev_os_head);

	IQOS_INFO("Device List populated.\n");
	return data;
}

/* Function get_category_json_object will gives category info in JSON format
	Parameters-----
	ptr_mac_address : Device MAC Address
	array_object    : A JSON array object to add category JSON object
	parameter_list	: parameters to include in output.
	app_cat_head	:
*/
static void
get_category_json_object(int device_index, json_object **array_object,
	struct list_head *parameter_list, struct list_head *app_cat_head)
{
	int itr_cat;
	char *app_cat = NULL;
	char buffer[IQOS_BUFF_SIZE] = { '\0' };
	struct iqos_category_node *cat_node = NULL;

	for (itr_cat = 0; itr_cat < IQOS_MAX_CAT_NODE; itr_cat++) {
		cat_node = g_device_info_collection[device_index]->cat_nodes[itr_cat];
		if (cat_node == NULL)
			continue;

		if (cat_node->active_flag == 0)
			continue;

		json_object *object = json_object_new_object();

		if (iqos_search_in_list(parameter_list,
			"Categories.Name")) {
				app_cat = search_app_cat(app_cat_head, cat_node->category_id);
				if (app_cat != NULL) {
					json_object_object_add(object, "Name",
						json_object_new_string(app_cat));
				}
		}

		if (iqos_search_in_list(parameter_list,
			"Categories.CategoryInt")) {
				json_object_object_add(object, "CategoryInt",
					json_object_new_int(cat_node->category_id));
		}

		if (iqos_search_in_list(parameter_list, "Categories.BW")) {
			json_object_object_add(object, "BW",
				json_object_new_string(iqos_lu_to_str(
				BYTES_TO_BITS(cat_node->cat_bandwidth),
				buffer, sizeof(buffer))));
		}

		if (iqos_search_in_list(parameter_list, "Categories.UpBytes")) {
			json_object_object_add(object, "UpBytes",
				json_object_new_string(iqos_lu_to_str(
				cat_node->accu_cat_up_bytes, buffer, sizeof(buffer))));
		}

		if (iqos_search_in_list(parameter_list, "Categories.DownBytes")) {
			json_object_object_add(object, "DownBytes",
				json_object_new_string(iqos_lu_to_str(
				cat_node->accu_cat_down_bytes, buffer, sizeof(buffer))));
		}
		json_object_array_add(*array_object, object);
	}
}

/*
	Function iqos_get_device_details will gives Devices details
	in JSON format for Device specified by MAC
	Parameters-----
	mac_addr_list	: MAC Address list for the devices whose details are needed
	parameter_list	: List of the parameters need to fetch in an JSON output
*/
char*
iqos_get_device_details(struct list_head *mac_addr_list, struct list_head *parameter_list)
{
	int ret = 0, refresh_rate = 0, device_index = -1, itr_device = 0;
	unsigned int itr_user_rec, itr_app_rec;
	unsigned long int total_down_byte = 0, total_up_byte = 0, app_count = 0;
	long unsigned int device_bandwidth = 0;
	long long unsigned int total_bw_bytes = 0, total_bw = 0;
	json_object	*object, *object_app, *array_app_list, *array_cat_list;
	char buffer[IQOS_BUFF_SIZE] = {'\0'};
	char *last_state = NULL, *category_name = NULL, *app_name = NULL;
	char mac_address[IQOS_BUFF_SIZE] = {'\0'};
	const char *ptr_device_details = NULL;
	struct timespec timeCurrent;
	long unsigned int app_duration = 0, app_total_bytes = 0, app_down_bandwidth = 0;
	long unsigned int last_download_bw_bytes = 0, last_upload_bw_bytes = 0;
	long unsigned int cur_download_bw = 0, cur_upload_bw =  0;
	long unsigned int last_app_total_down_bytes = 0;
	iqos_tokens_t *mac_addr = NULL;
	devid_user_e *usr_lst = NULL;
	devid_app_rate_e *app_lst = NULL;
	struct list_head *pos, *next;
	LIST_HEAD(app_inf_head);
	LIST_HEAD(app_cat_head);

	list_for_each_safe(pos, next, mac_addr_list) {
		mac_addr = list_entry(pos, iqos_tokens_t, list);
		if (iqos_inline_url_unescape(mac_addr->value) < 0)
			return NULL;
	}

	clock_gettime(CLOCK_REALTIME, &timeCurrent);
	if (g_timestamp_last_called.tv_sec == 0) {
		g_timestamp_last_called.tv_sec = timeCurrent.tv_sec;
		refresh_rate = IQOS_MIN_REFRESH_RATE;
	} else {
		refresh_rate = timeCurrent.tv_sec - g_timestamp_last_called.tv_sec;
		if (refresh_rate < IQOS_MIN_REFRESH_RATE) {
			last_state = iqos_get_device_last_state(mac_addr_list);
			IQOS_INFO(" Refresh Rate is less than standard So Last state evaluated.\n");
			return last_state;
		}
		g_timestamp_last_called.tv_sec = timeCurrent.tv_sec;
	}

	ret = GET_FW_USERLIST(&usr_lst);

	if (ret < 0) {
		if (usr_lst) {
			free(usr_lst);
			usr_lst = NULL;
		}
		IQOS_ERROR("Unable to populate User List.\n");
		return NULL;
	}

	ret = get_fw_user_app_rate(&app_lst);
	if (ret < 0) {
		if (usr_lst) {
			free(usr_lst);
			usr_lst = NULL;
		}

		if (app_lst) {
			free(app_lst);
			app_lst = NULL;
		}
		IQOS_ERROR("Unable to populate Application List.\n");
		return NULL;
	}

	if (!usr_lst || !app_lst) {
		if (usr_lst) {
			free(usr_lst);
			usr_lst = NULL;
		}

		if (app_lst) {
			free(app_lst);
			app_lst = NULL;
		}
		IQOS_ERROR("Unable to populate Application List Or User List.\n");
		return NULL;
	}

	iqos_validate_devices(usr_lst);
	total_bw_bytes = iqos_get_total_nw_bandwidth(usr_lst, app_lst);

	init_app_inf(&app_inf_head);
	init_app_cat(&app_cat_head);
	iqos_set_category_cleanup();

	for (itr_user_rec = 0; itr_user_rec < DEVID_MAX_USER; itr_user_rec++) {
		total_down_byte = 0, last_download_bw_bytes = 0;
		total_up_byte = 0, last_upload_bw_bytes = 0;
		buffer[0] = '\0';
		mac_address[0] = '\0';
		device_bandwidth = 0;
		device_index = -1;
		app_count = 0;

		if (usr_lst[itr_user_rec].available <= 0) {
			break;
		}

		snprintf(buffer, sizeof(buffer), MAC_OCTET_FMT,
			MAC_OCTET_EXPAND(usr_lst[itr_user_rec].mac));

		if (strlen(buffer) == 0)
			continue;

		strncpy_n(mac_address, buffer, sizeof(buffer));

		if (!iqos_find_device(mac_address, &device_index)) {
			for (itr_device = 0; itr_device < DEVID_MAX_USER; itr_device++) {
				if (g_device_info_collection[itr_device] == NULL) {
					iqos_add_device(mac_address, itr_device);
					device_index = itr_device;
					break;
				}
			}
		}
		if (device_index <= -1)
			continue;

		object = json_object_new_object();
		array_app_list = json_object_new_array();
		array_cat_list = json_object_new_array();

		for (itr_app_rec = 0; itr_app_rec < DEVID_APP_RATE_TABLE_POOL_SIZE;
			itr_app_rec++) {

			if (app_lst[itr_app_rec].available <= 0) {
				break;
			}

			if (usr_lst[itr_user_rec].uid != app_lst[itr_app_rec].uid) {
				continue;
			}

			object_app = json_object_new_object();

			if (iqos_search_in_list(parameter_list, "Apps.CategoryInt")) {
				json_object_object_add(object_app, "CategoryInt",
					json_object_new_int(app_lst[itr_app_rec].cat_id));
			}

			category_name = search_app_cat(&app_cat_head, app_lst[itr_app_rec].cat_id);
			if (category_name != NULL) {
				if (iqos_search_in_list(parameter_list, "Apps.Category")) {
					json_object_object_add(object_app, "Category",
						json_object_new_string(category_name));
				}
			}

			app_name = search_app_inf(&app_inf_head,
			app_lst[itr_app_rec].cat_id, app_lst[itr_app_rec].app_id);

			if (app_name != NULL) {
				if (iqos_search_in_list(parameter_list, "Apps.Name"))
					json_object_object_add(object_app, "Name",
						json_object_new_string(app_name));
			}

			snprintf(buffer, sizeof(buffer), "%u", APP_DL_PKT(app_lst, itr_app_rec));
			if (iqos_search_in_list(parameter_list, "Apps.DownPkts")) {
				json_object_object_add(object_app, "DownPkts",
					json_object_new_string(buffer));
			}

			snprintf(buffer, sizeof(buffer), "%u",
				APP_UL_PKT(app_lst, itr_app_rec));

			if (iqos_search_in_list(parameter_list, "Apps.UpPkts")) {
				json_object_object_add(object_app, "UpPkts",
					json_object_new_string(buffer));
			}

			app_duration = app_lst[itr_app_rec].last_elapsed_ts;

			if (iqos_search_in_list(parameter_list, "Apps.LastUsed")) {
				json_object_object_add(object_app, "LastUsed",
					json_object_new_string(iqos_lu_to_str(
					app_duration, buffer, sizeof(buffer))));
			}

			if (iqos_search_in_list(parameter_list, "Apps.DownBytes")) {
				json_object_object_add(object_app, "DownBytes",
					json_object_new_string(iqos_lu_to_str(
						APP_DL_BYTE(app_lst, itr_app_rec),
						buffer, sizeof(buffer))));
			}

			if (iqos_search_in_list(parameter_list, "Apps.UpBytes")) {
				json_object_object_add(object_app, "UpBytes",
					json_object_new_string(iqos_lu_to_str(
						APP_UL_BYTE(app_lst, itr_app_rec),
						buffer, sizeof(buffer))));
			}

			app_total_bytes = (APP_DL_BYTE(app_lst, itr_app_rec) +
				APP_UL_BYTE(app_lst, itr_app_rec));

			last_app_total_down_bytes = iqos_find_app_node(mac_address,
				app_lst[itr_app_rec].cat_id, app_lst[itr_app_rec].app_id,
				app_total_bytes, APP_DL_BYTE(app_lst, itr_app_rec),
				&app_duration);

			if (APP_DL_BYTE(app_lst, itr_app_rec) >=  last_app_total_down_bytes) {
				app_down_bandwidth = (APP_DL_BYTE(app_lst, itr_app_rec) -
					last_app_total_down_bytes) / refresh_rate;
			} else {
				app_down_bandwidth = APP_DL_BYTE(app_lst, itr_app_rec) /
					refresh_rate;
			}

			if (iqos_search_in_list(parameter_list, "Apps.BW") != 0) {
				json_object_object_add(object_app, "BW", json_object_new_string(
						iqos_lu_to_str(BYTES_TO_BITS(app_down_bandwidth),
						buffer, sizeof(buffer))));
			}

			iqos_update_category_info(device_index, app_lst[itr_app_rec].cat_id,
				APP_UL_BYTE(app_lst, itr_app_rec),
				APP_DL_BYTE(app_lst, itr_app_rec));

			total_down_byte += APP_DL_BYTE(app_lst, itr_app_rec);
			total_up_byte += APP_UL_BYTE(app_lst, itr_app_rec);

			json_object_array_add(array_app_list, object_app);

			app_count++;
		}
		iqos_update_device_info(device_index, total_up_byte,
			total_down_byte);
		json_object_object_add(object, "MacAddr", json_object_new_string(mac_address));
		json_object_object_add(object, "Apps", array_app_list);

		iqos_get_device_bandwidth_bytes(device_index,
			&last_download_bw_bytes, &last_upload_bw_bytes);

		cur_download_bw = last_download_bw_bytes / refresh_rate;

		if (iqos_search_in_list(parameter_list, "DownBW")) {
			json_object_object_add(object, "DownBW",
				json_object_new_string(iqos_lu_to_str(
					BYTES_TO_BITS(cur_download_bw),	buffer, sizeof(buffer))));
		}

		cur_upload_bw = last_upload_bw_bytes / refresh_rate;

		if (iqos_search_in_list(parameter_list, "UpBW")) {
			json_object_object_add(object, "UpBW",
				json_object_new_string(iqos_lu_to_str(BYTES_TO_BITS(cur_upload_bw),
				buffer, sizeof(buffer))));
		}

		if (iqos_search_in_list(parameter_list, "RefreshInterval")) {
			json_object_object_add(object, "RefreshInterval",
				json_object_new_int(refresh_rate));
		}

		if (iqos_search_in_list(parameter_list, "AppCount")) {
			json_object_object_add(object, "AppCount",
				json_object_new_string(iqos_lu_to_str(app_count,
				buffer, sizeof(buffer))));
		}

		total_bw = total_bw_bytes > 0 ? (total_bw_bytes / refresh_rate) : 0;

		if (iqos_search_in_list(parameter_list, "BW")) {
			json_object_object_add(object, "BW",
				json_object_new_string(iqos_llu_to_str(
					BYTES_TO_BITS(total_bw), buffer, sizeof(buffer))));
		}

		if (iqos_search_in_list(parameter_list, "DownBytes")) {
			json_object_object_add(object, "DownBytes",
				json_object_new_string(iqos_lu_to_str(last_download_bw_bytes,
				buffer, sizeof(buffer))));
		}

		if (iqos_search_in_list(parameter_list, "UpBytes")) {
			json_object_object_add(object, "UpBytes",
				json_object_new_string(iqos_lu_to_str(last_upload_bw_bytes,
				buffer, sizeof(buffer))));
		}

		iqos_update_category_collection(device_index, refresh_rate);
		get_category_json_object(device_index, &array_cat_list,
			parameter_list, &app_cat_head);

		json_object_object_add(object, "Categories", array_cat_list);
		ptr_device_details = json_object_to_json_string(object);

		iqos_set_device_state(device_index, ptr_device_details);
		json_object_put(object);
	}

	iqos_remove_category_nodes();
	iqos_remove_dead_devices();
	last_state = iqos_get_device_last_state(mac_addr_list);

error:
	free_app_inf(&app_inf_head);
	free_app_cat(&app_cat_head);

	if (app_lst) {
		free(app_lst);
	}
	if (usr_lst) {
		free(usr_lst);
	}

	IQOS_INFO("Device Details populated.\n");
	return last_state;
}

/*
	Retrieves devices last state
Parameters-----
	 mac_Addresses	: List of MAC Addresses whose last state to fetch[in]
	 Return value	: Device last state
*/
static char*
iqos_get_device_last_state(struct list_head *mac_addrs)
{
	int itr_dev_node;
	char* data = NULL;
	const char* device_state = NULL;
	struct iqos_device_info *device_info = NULL;

	json_object *array_device_object = json_object_new_array();
	json_object *device_object = json_object_new_object();
	json_object *device_temp_object = NULL;
	json_object_object_add(device_object, "Req", json_object_new_string("DeviceSummary"));

	for (itr_dev_node = 0; itr_dev_node < IQOS_MAX_DEVICES; itr_dev_node++) {
		device_info = g_device_info_collection[itr_dev_node];

		if (device_info == NULL)
			continue;

		if (iqos_search_in_list(mac_addrs, device_info->mac_address)) {
			if (device_info->device_previous_state) {
				device_temp_object = json_tokener_parse(
					device_info->device_previous_state);
				json_object_array_add(array_device_object, device_temp_object);
			}
		}
	}

	json_object_object_add(device_object, "Device", array_device_object);
	device_state = json_object_to_json_string(device_object);
	data = strdup(device_state);
	json_object_put(device_object);
	return data;
}

/*
	Stores device last state
Parameters-----
	 device_index : Device index in device collection whose detail need to store [in]
	 device_details : Device state to be stored [in]
*/
static void
iqos_set_device_state(int device_index, const char *device_details)
{
	struct iqos_device_info *device_info = g_device_info_collection[device_index];
	if (device_info->device_previous_state != NULL) {
		free(device_info->device_previous_state);
	}
	device_info->device_previous_state = strdup(device_details);
}

/* Write json answer to stream */
void
do_iqos_get(char *url, FILE *stream)
{
	if (g_iqos_output_buffer != NULL)
		fputs(g_iqos_output_buffer, stream);

	if (g_iqos_output_buffer != NULL) {
		free(g_iqos_output_buffer);
		g_iqos_output_buffer = NULL;
	}
}

/* Read query from stream in json format treate it and create answer string
 * The query is a list of commands and treated by 'do_json_command'
 */
void
do_iqos_post(const char *orig_url, FILE *stream, int len, const char *boundary)
{
	char *ptr;
	int reqtype;
	char *url;
	int ret = 0;
	LIST_HEAD(nvlist);
	char *dbg_level = NULL;

	if (first_execution == 1) {
		dbg_level = nvram_get("iqos_debug_level");
		iqos_debug_flag = dbg_level ? atoi(dbg_level) : IQOS_DEBUG_ERROR;
		IQOS_INFO("Value Of iqos_debug_flag = [%d]\n", iqos_debug_flag);
		first_execution = 0;
	}

	/* Sample URL = "iqos.cgi?Req=DeviceList&fields=Name,DevType,MacAddr&
	 * MacAddr=00:11:22:33:44:55,01:0b:0c:0d:0e:0f,aa:bb:cc:dd:ee:ff"
	 */
	g_iqos_restart_flag = IQOS_NOOP;
	if (orig_url == NULL) {
		IQOS_ERROR("Null URL for iQoS UI Request\n");
		return;
	}
	url = strdup(orig_url);

	/* Validate the URL and get the URL parameters pointer */
	ptr = iqos_validate_url(url);
	if (ptr == NULL) {
		free(url);
		return;
	}

	/* Tokenize to name=value pair */
	iqos_split_string(ptr, "&", &nvlist);
	iqos_print_tokens(&nvlist, "NVPair");

	/* Identify the Request */
	reqtype = iqos_get_request_type(&nvlist);
	if (reqtype < 0) {
		IQOS_ERROR("Bad iQoS UI Request [%s]\n", orig_url);
		free(url);
		return;
	}

	if (g_iqos_output_buffer != NULL) {
		free(g_iqos_output_buffer);
		g_iqos_output_buffer = NULL;
	}

	/* Process the Request */
	switch (reqtype) {
		case IQOS_UI_REQ_TYPE_DEVICE_LIST:
			ret = iqos_process_req_devicelist(&nvlist);
			break;
		case IQOS_UI_REQ_TYPE_DEVICE_SUMMARY:
			ret = iqos_process_req_devicesummary(&nvlist);
			break;
		case IQOS_UI_REQ_TYPE_GET_CONFIG:
			ret = iqos_process_req_getconfig(&nvlist);
			break;
		case IQOS_UI_REQ_TYPE_SET_CONFIG:
			ret = iqos_process_req_setconfig(&nvlist, stream, len, boundary);
			break;
		case IQOS_UI_REQ_TYPE_GET_DEV_TYPE_PRIO:
			ret = iqos_process_req_getdevtypeprio(&nvlist);
			break;
		case IQOS_UI_REQ_TYPE_SET_DEV_TYPE_PRIO:
			ret = iqos_process_req_setdevtypeprio(&nvlist, stream, len, boundary);
			break;
		case IQOS_UI_REQ_TYPE_GET_APP_CAT_PRIO:
			ret = iqos_process_req_getappcatprio(&nvlist);
			break;
		case IQOS_UI_REQ_TYPE_SET_APP_CAT_PRIO:
			ret = iqos_process_req_setappcatprio(&nvlist, stream, len, boundary);
			break;
		case IQOS_UI_REQ_TYPE_GET_MAC_PRIO:
			ret = iqos_process_req_getmacprio(&nvlist);
			break;
		case IQOS_UI_REQ_TYPE_SET_MAC_PRIO:
			ret = iqos_process_req_setmacprio(&nvlist, stream, len, boundary);
			break;
		case IQOS_UI_REQ_TYPE_SET_DEFAULT_PRIO:
			ret = iqos_process_req_setdefaultprio(&nvlist, stream, len, boundary);
			break;
		case IQOS_UI_REQ_TYPE_SET_ALL_PRIO:
			ret = iqos_process_req_setallprio(&nvlist, stream, len, boundary);
			break;
	}

	iqos_free_token_list(&nvlist);

	/* If iqosd is getting stopped / started / restarted, let us commit all the settings */
	if (g_iqos_restart_flag) {
		if (iqos_is_default_conf() == 1) {
			iqos_set_default_conf(0);
		}
		iqos_file_backup();
	}

	/* Stop / Restart / Start iQoS daemon, based on the processing done above */
	if (g_iqos_restart_flag & IQOS_STOP)
		iqos_enable(IQOS_STOP);
	else if (g_iqos_restart_flag & IQOS_RESTART)
		iqos_enable(IQOS_RESTART);
	else if (g_iqos_restart_flag & IQOS_START)
		iqos_enable(IQOS_START);
	g_iqos_restart_flag = IQOS_NOOP;
	free(url);
}

/* Place a file lock */
static int
iqos_file_lock(FILE *fp, int op)
{
	int i;

	for (i = 0; i < FILE_LOCK_RETRY; i++) {
		if (flock(fileno(fp), op))
			usleep(100000);
		else
			return 0;
	}
	IQOS_ERROR("iqos_file_lock: flock error\n");
	return -1;
}

/* Get infomation of iqos enabled/disabled
 *
 * @return	1 if iQos enabled, 0 if iQos disabled.
 */
int
iqos_is_enabled(void)
{
	return (nvram_match("broadstream_iqos_enable", "1"));
}

/* start | restart | stop iqos.
 *
 * @param	enable	IQOS_START | IQOS_RESTART | IQOS_STOP
 * @return	0 if success, -1 if fail.
 */
int
iqos_enable(int enable)
{
	int ret = 0;

	switch (enable) {
	case IQOS_START:
		if (!nvram_match("broadstream_iqos_enable", "1")) {
			nvram_set("broadstream_iqos_enable", "1");
			nvram_commit();
			eval("bcmiqosd", "start");
		}
		break;
	case IQOS_RESTART:
		if (!nvram_match("broadstream_iqos_enable", "1")) {
			nvram_set("broadstream_iqos_enable", "1");
			nvram_commit();
		}
		eval("bcmiqosd", "restart");
		break;
	case IQOS_STOP:
		if (nvram_match("broadstream_iqos_enable", "1")) {
			eval("bcmiqosd", "stop");
		}
		nvram_set("broadstream_iqos_enable", "0");
		nvram_commit();
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

/* To Know whether WAN port bandwidth auto-detection is configured or not
 *
 * @return	1 if confiured as auto-detecting, 0 otherwise.
 */
int
iqos_is_wan_bw_auto(void)
{
	return (nvram_match("broadstream_iqos_wan_bw_auto", "1"));
}

/* Enable/disable WAN port bandwidth auto-detection
 *
 * @param	enabel	1|0
 * @return	0 if success, -1 if fail.
 */
int
iqos_set_wan_bw_auto(int enable)
{
	if (nvram_match("broadstream_iqos_wan_bw_auto", "1")) {
		if (enable == 0) {
			nvram_set("broadstream_iqos_wan_bw_auto", "0");
		} else {
			return 0;
		}
	} else {
		if (enable == 0) {
			nvram_set("broadstream_iqos_wan_bw_auto", "0");
		} else {
			nvram_set("broadstream_iqos_wan_bw_auto", "1");
		}
	}

	nvram_commit();

	return 0;
}

/* Get upload bandwidth and download bandwidth configured in iQos
 *
 * @param	upbw	fill upload bandwidth in upbw
 * @param	downbw	fill download bandwidth in downbw
 * @return	0 if success, others if fail
 */
int
iqos_get_wan_bw(int *upbw, int *downbw)
{
	FILE *fp;
	char line[256];
	char *end = NULL;
	int ret = 0;

	*upbw = *downbw = 0;

	if (!(fp = fopen(PATH_QOS_CONF, "r"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
		return -1;
	}

	if (iqos_file_lock(fp, LOCK_SH | LOCK_NB)) {
		fclose(fp);
		return -1;
	}

	while (fgets(line, 255, fp)) {
		if (strncmp(line, "ceil_down=", strlen("ceil_down=")) == 0) {
			if ((end = strstr(line, "kbps")) == NULL) {
				IQOS_ERROR("Incorrect Content: %s \n", PATH_QOS_CONF);
				ret = -1;
				break;
			}
			*end = '\0';
			*downbw = atoi(&line[strlen("ceil_down=")]);
			if ((*downbw > 0) && (*upbw > 0))
				break;
		} else if (strncmp(line, "ceil_up=", strlen("ceil_up=")) == 0) {
			if ((end = strstr(line, "kbps")) == NULL) {
				IQOS_ERROR("Incorrect Content: %s \n", PATH_QOS_CONF);
				ret = -1;
				break;
			}
			*end = '\0';
			*upbw = atoi(&line[strlen("ceil_up=")]);
			if ((*downbw > 0) && (*upbw > 0))
				break;
		}
	}

	flock(fileno(fp), LOCK_UN | LOCK_NB);
	fclose(fp);

	if ((*downbw <= 0) || (*upbw <= 0)) {
		IQOS_ERROR("Incorrect Content: %s \n", PATH_QOS_CONF);
		ret = -1;
	}

	return ret;
}

/* Set upload bandwidth and download bandwidth into iQos
 *
 * @param	upbw	upload bandwidth (Kbyte/s)
 * @param	downbw	download bandwidth (Kbyte/s)
 * @return	0 if success, others if fail
 */
int
iqos_set_wan_bw(int upbw, int downbw)
{
	char line[256], download[32], upload[32];
	FILE *fp1 = NULL, *fp2 = NULL;
	const char *const_down_str = "ceil_down=";
	const char *const_up_str = "ceil_up=";
	char *buf = NULL;
	int err = -1, size;

	snprintf(upload, sizeof(upload) - 1, "%d", upbw);
	snprintf(download, sizeof(download) - 1, "%d", downbw);

	if (!(fp1 = fopen(PATH_QOS_CONF, "r+"))) {
		if (iqos_file_restore())
			iqos_load_default();

		if (!(fp1 = fopen(PATH_QOS_CONF, "r+"))) {
			IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
			goto out;
		}
	}

	if (!(fp2 = fopen(PATH_QOS_CONF_TMP, "w+"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF_TMP, strerror(errno));
		goto out;
	}

	if (iqos_file_lock(fp1, LOCK_EX | LOCK_NB))
		goto out;

	if (fseek(fp1, 0, SEEK_END) || ((size = ftell(fp1)) < 0))
		goto out;
	rewind(fp1);

	if (!(buf = (char *)malloc(size)))
		goto out;

	if ((fread(buf, 1, size, fp1) < size) || (fwrite(buf, 1, size, fp2) < size))
		goto out;
	rewind(fp1);
	rewind(fp2);

	if (ftruncate(fileno(fp1), 0) < 0)
		goto out;

	while (fgets(line, sizeof(line) - 1, fp2)) {
		if (strncmp(line, "ceil_down=", strlen("ceil_down=")) == 0)
			fprintf(fp1, "%s%s%s\n", const_down_str, download, "kbps");
		else if (strncmp(line, "ceil_up=", strlen("ceil_up=")) == 0)
			fprintf(fp1, "%s%s%s\n", const_up_str, upload, "kbps");
		else {
			if (fputs(line, fp1) < 0)
				goto out;
		}
	}

	flock(fileno(fp1), LOCK_UN | LOCK_NB);
	err = 0;
out:
	if (fp1)
		fclose(fp1);
	if (fp2) {
		fclose(fp2);
		if (unlink(PATH_QOS_CONF_TMP))
			IQOS_ERROR("unlink %s Due to %s\n", PATH_QOS_CONF_TMP, strerror(errno));
	}
	if (buf)
		free(buf);
	return err;
}

/* Get priority of application category
 *
 * @param	appcatid	application category ID
 * @return	priority of application category, -1 if fail
 */
int
iqos_get_apps_priority(int appcatid)
{
	FILE *fp;
	char line[256];
	char *start, *end;
	int priority = -1, ret = -1;

	if (!(fp = fopen(PATH_QOS_CONF, "r"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
		return -1;
	}

	if (iqos_file_lock(fp, LOCK_SH | LOCK_NB)) {
		fclose(fp);
		return -1;
	}

	while (fgets(line, 255, fp)) {
		if (strchr(line, '#') != NULL) {
			continue;
		} else if ((start = strchr(line, '[')) != NULL) {
			if ((end = strchr(line, ']')) != NULL) {
				*end = '\0';
				priority = atoi(&start[1]);
			} else {
				break;
			}
		} else if (strncmp(line, "rule=", strlen("rule=")) == 0) {
			if (atoi(&line[strlen("rule=")]) == appcatid) {
				if (priority >= 0)
					ret = priority;
				break;
			}
		}
	}

	flock(fileno(fp), LOCK_UN | LOCK_NB);

	fclose(fp);

	return ret;
}

/* Set priority of application category
 *
 * @param	appcatid	application category ID
 * @param	prio		priority of application category
 * @return	0 if success, -1 if fail
 */
int
iqos_set_apps_priority(int appcatid, int prio)
{
	char line[256];
	char *start, *end, *buf = NULL;
	FILE *fp1 = NULL, *fp2 = NULL;
	int priority, err = -1, size;

	if (!(fp1 = fopen(PATH_QOS_CONF, "r+"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
		goto out;
	}

	if (!(fp2 = fopen(PATH_QOS_CONF_TMP, "w+"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF_TMP, strerror(errno));
		goto out;
	}

	if (iqos_file_lock(fp1, LOCK_EX | LOCK_NB))
		goto out;

	if (fseek(fp1, 0, SEEK_END) || ((size = ftell(fp1)) < 0))
		goto out;
	rewind(fp1);

	if (!(buf = (char *)malloc(size)))
		goto out;

	if ((fread(buf, 1, size, fp1) < size) || (fwrite(buf, 1, size, fp2) < size))
		goto out;
	rewind(fp1);
	rewind(fp2);

	if (ftruncate(fileno(fp1), 0) < 0)
		goto out;

	while (fgets(line, sizeof(line) - 1, fp2)) {
		if (line[0] == '#') {
			if (fputs(line, fp1) < 0)
				goto out;
			continue;
		}
		if ((start = strchr(line, '[')) != NULL) {
			if (fputs(line, fp1) < 0)
				goto out;
			if ((end = strchr(line, ']')) != NULL) {
				*end = '\0';
				priority = atoi(&start[1]);
				if (priority == prio)
					fprintf(fp1, "rule=%d\n", appcatid);
			}
		} else if ((strncmp(line, "rule=", strlen("rule=")) == 0) &&
			(atoi(&line[strlen("rule=")]) == appcatid)) {
			continue;
		} else {
			if (fputs(line, fp1) < 0)
				goto out;
		}
	}

	flock(fileno(fp1), LOCK_UN | LOCK_NB);

	err = 0;
out:
	if (fp1)
		fclose(fp1);
	if (fp2) {
		fclose(fp2);
		if (unlink(PATH_QOS_CONF_TMP))
			IQOS_ERROR("unlink %s Due to %s\n", PATH_QOS_CONF_TMP, strerror(errno));
	}
	if (buf)
		free(buf);
	return err;
}

/* Get priority of device category
 *
 * @param	devcatid	device category ID
 * @return	priority of device category if success, -1 if fail
 */
int
iqos_get_devs_priority(int devcatid)
{
	FILE *fp;
	char line[256];
	char *start, *end;
	int priority = -1, ret = -1;

	if (!(fp = fopen(PATH_QOS_CONF, "r"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
		return -1;
	}

	if (iqos_file_lock(fp, LOCK_SH | LOCK_NB)) {
		fclose(fp);
		return -1;
	}

	while (fgets(line, 255, fp)) {
		if (strchr(line, '#') != NULL) {
			continue;
		} else if ((start = strchr(line, '{')) != NULL) {
			if ((end = strchr(line, '}')) != NULL) {
				*end = '\0';
				priority = atoi(&start[1]);
			} else {
				break;
			}
		} else if (strncmp(line, "cat=", strlen("cat=")) == 0) {
			if (atoi(&line[strlen("cat=")]) == devcatid) {
				if (priority >= 0)
					ret = priority;
				break;
			}
		}
	}

	flock(fileno(fp), LOCK_UN | LOCK_NB);

	fclose(fp);

	return ret;
}

/* Set priority of device category
 *
 * @param	devcatid	device category ID
 * @param	prio		priority of device category
 * @return	0 if success, -1 if fail
 */
int
iqos_set_devs_priority(int devcatid, int prio)
{
	char line[256];
	char *start, *end, *buf = NULL;
	FILE *fp1 = NULL, *fp2 = NULL;
	int priority, err = -1, size;

	if (!(fp1 = fopen(PATH_QOS_CONF, "r+"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
		goto out;
	}
	if (!(fp2 = fopen(PATH_QOS_CONF_TMP, "w+"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF_TMP, strerror(errno));
		goto out;
	}

	if (iqos_file_lock(fp1, LOCK_EX | LOCK_NB))
		goto out;

	if (fseek(fp1, 0, SEEK_END) || ((size = ftell(fp1)) < 0))
		goto out;
	rewind(fp1);

	if (!(buf = (char *)malloc(size)))
		goto out;

	if ((fread(buf, 1, size, fp1) < size) || (fwrite(buf, 1, size, fp2) < size))
		goto out;
	rewind(fp1);
	rewind(fp2);

	if (ftruncate(fileno(fp1), 0) < 0)
		goto out;

	while (fgets(line, sizeof(line) - 1, fp2)) {
		/* Skip commented lines */
		if (line[0] == '#') {
			if (fputs(line, fp1) < 0)
				goto out;
			continue;
		}
		if ((start = strchr(line, '{')) != NULL) {
			if (fputs(line, fp1) < 0)
				goto out;
			if ((end = strchr(line, '}')) != NULL) {
				*end = '\0';
				priority = atoi(&start[1]);
				if (priority == prio)
					fprintf(fp1, "cat=%d\n", devcatid);
			}
		} else if ((strncmp(line, "cat=", strlen("cat=")) == 0) &&
			(atoi(&line[strlen("cat=")]) == devcatid)) {
			continue;
		} else {
			if (fputs(line, fp1) < 0)
				goto out;
		}
	}

	flock(fileno(fp1), LOCK_UN | LOCK_NB);

	err = 0;
out:
	if (fp1)
		fclose(fp1);
	if (fp2) {
		fclose(fp2);
		if (unlink(PATH_QOS_CONF_TMP))
			IQOS_ERROR("unlink %s Due to %s\n", PATH_QOS_CONF_TMP, strerror(errno));
	}
	if (buf)
		free(buf);
	return err;
}

/* Get a list of MAC address & priority pairs that have been configured in qos.conf
 *
 * @param	buf	pointer of buf which id for containing MAC address & priority pairs
 * @param	size	maximum size of buf
 * @return	number of MAC address & priority pairs filled in the buf, return -1 if fail
 */
int
iqos_list_mac_priority(iqos_mac_priority_t *buf, int size)
{
	FILE *fp;
	char line[256], cmp[5] = "mac=";
	char *start, *end, *p, *t;
	int priority = -1, counter = 0, i;

	if (!(fp = fopen(PATH_QOS_CONF, "r"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
		return -1;
	}

	if (iqos_file_lock(fp, LOCK_SH | LOCK_NB)) {
		fclose(fp);
		return -1;
	}

	while (fgets(line, 255, fp)) {
		if (strchr(line, '#') != NULL) {
			continue;
		} else if ((start = strchr(line, '{')) != NULL) {
			if ((end = strchr(line, '}')) != NULL) {
				*end = '\0';
				priority = atoi(&start[1]);
			} else {
				counter = -1;
				break;
			}
		} else if (strncmp(line, cmp, strlen(cmp)) == 0) {
			if (priority >= 0) {
				p = &line[strlen(cmp)];
				t = buf[counter].macaddr;
				for (i = 0; i < ETHER_ADDR_LEN; i++) {
					if (i)
						*t++ = ':';
					strncpy(t, p, 2);
					t += 2;
					p += 2;
				}
				*t = '\0';
				buf[counter].prio = priority;
				counter++;
				if (counter >= size)
					break;
			}
		}
	}

	flock(fileno(fp), LOCK_UN | LOCK_NB);

	fclose(fp);

	return counter;
}

/* Get priority of device configured by MAC address
 *
 * @param	macaddr		device MAC address
 * @return	priority of device if defined, others return -1
 */
int
iqos_get_mac_priority(char *macaddr)
{
	FILE *fp;
	unsigned char ea[ETHER_ADDR_LEN];
	char line[256], eabuf[32], cmp[32] = "mac=";
	char *start, *end;
	int priority = -1, ret = -1;
	int i;
	int index = 0;

	if (!(fp = fopen(PATH_QOS_CONF, "r"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
		return -1;
	}

	ether_atoe(macaddr, ea);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		index += snprintf(&eabuf[index], sizeof(eabuf) - index, "%02X", ea[i] & 0xff);
	}

	strncat(cmp, eabuf, sizeof(cmp) - strlen(cmp) - 1);

	if (iqos_file_lock(fp, LOCK_SH | LOCK_NB)) {
		fclose(fp);
		return -1;
	}

	while (fgets(line, 255, fp)) {
		if (strchr(line, '#') != NULL) {
			continue;
		} else if ((start = strchr(line, '{')) != NULL) {
			if ((end = strchr(line, '}')) != NULL) {
				*end = '\0';
				priority = atoi(&start[1]);
			} else {
				break;
			}
		} else if (strncmp(line, cmp, strlen(cmp)) == 0) {
			if (priority >= 0)
				ret = priority;
			break;
		}
	}

	flock(fileno(fp), LOCK_UN | LOCK_NB);

	fclose(fp);

	return ret;
}

/* Wrapper on top of iqos_get_mac_priority.
 * If the mac priority is not set for this device, this will send
 * out the corresponding device type priority
 */
static int
iqos_get_mac_priority_ex(char *macaddr)
{
	int priority = -1;
	int devtype = -1;

	priority = iqos_get_mac_priority(macaddr);

	/* Check if a proper priority is received. If so, return it */
	if (priority >= 0)
		return priority;

	/* This device has no mac based priority set. So, let us find out which device type
	 * this mac address belong to and then send out the corresponding priority
	 */
	devtype = iqos_get_devtype(macaddr);
	priority = iqos_get_devs_priority(devtype);
	return priority;
}

/* Get the device type based on the mac address */
static int
iqos_get_devtype(char *macaddr)
{
	devid_user_e *usr_lst = NULL;
	int typeid = -1;
	int ret = -1, i;
	char dev_mac[IQOS_MACSTR_LEN];

	/* Get list of all devices active on the network */
	ret = GET_FW_USERLIST(&usr_lst);

	if (ret < 0) {
		if (usr_lst) {
			free(usr_lst);
			usr_lst = NULL;
		}
		return -1;
	}

	/* run through the list until we match the requested mac id */
	for (i = 0; i < DEVID_MAX_USER; i++) {
		if (usr_lst[i].available <= 0)
			break;

		snprintf(dev_mac, sizeof(dev_mac), MAC_OCTET_FMT, MAC_OCTET_EXPAND(usr_lst[i].mac));
		if (strcasecmp(dev_mac, macaddr) == 0) {
			/* Found the device. Get it's typeid */
			typeid = USR_TYPE_ID(i);
			break;
		}
	}

	if (usr_lst)
		free(usr_lst);
	return typeid;
}

/* Set priority of device by MAC address
 *
 * @param	macaddr		string of device MAC address (XX:XX:XX:XX:XX:XX)
 * @param	prio		priority of deivce
 * @return	0 if success, -1 if fail
 */
int
iqos_set_mac_priority(char *macaddr, int prio)
{
	FILE *fp1 = NULL, *fp2 = NULL;
	unsigned char ea[ETHER_ADDR_LEN];
	char line[256], eabuf[32];
	char *c, *start, *end, *buf = NULL;
	int priority, i, err = -1, size;

	if (!(fp1 = fopen(PATH_QOS_CONF, "r+"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF, strerror(errno));
		goto out;
	}
	if (!(fp2 = fopen(PATH_QOS_CONF_TMP, "w+"))) {
		IQOS_ERROR("fopen %s Due to %s\n", PATH_QOS_CONF_TMP, strerror(errno));
		goto out;
	}

	if (iqos_file_lock(fp1, LOCK_EX | LOCK_NB))
		goto out;

	if (fseek(fp1, 0, SEEK_END) || ((size = ftell(fp1)) < 0))
		goto out;
	rewind(fp1);

	if (!(buf = (char *)malloc(size)))
		goto out;

	if ((fread(buf, 1, size, fp1) < size) || (fwrite(buf, 1, size, fp2) < size))
		goto out;
	rewind(fp1);
	rewind(fp2);

	if (ftruncate(fileno(fp1), 0) < 0)
		goto out;

	ether_atoe(macaddr, ea);

	c = eabuf;
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		c += snprintf(c, sizeof(char) * 3, "%02X", ea[i] & 0xff);

	while (fgets(line, sizeof(line) - 1, fp2)) {
		/* Skip commented lines */
		if (line[0] == '#') {
			if (fputs(line, fp1) < 0)
				goto out;
			continue;
		}
		if ((start = strchr(line, '{')) != NULL) {
			if (fputs(line, fp1) < 0)
				goto out;
			if ((end = strchr(line, '}')) != NULL) {
				*end = '\0';
				priority = atoi(&start[1]);
				if (priority == prio)
					fprintf(fp1, "mac=%s\n", eabuf);
			}
		} else if ((strncmp(line, "mac=", strlen("mac=")) == 0) &&
			(strncmp(&line[strlen("mac=")], eabuf, strlen(eabuf)) == 0)) {
			continue;
		} else {
			if (fputs(line, fp1) < 0)
				goto out;
		}
	}

	flock(fileno(fp1), LOCK_UN | LOCK_NB);

	err = 0;
out:
	if (fp1)
		fclose(fp1);
	if (fp2) {
		fclose(fp2);
		if (unlink(PATH_QOS_CONF_TMP))
			IQOS_ERROR("unlink %s Due to %s\n", PATH_QOS_CONF_TMP, strerror(errno));
	}
	if (buf)
		free(buf);
	return err;
}

/* To know whether apply default iQos configuration file or not
 *
 * @return	1 denotes to apply default iQos configuration file, 0 otherwise.
 */
int
iqos_is_default_conf(void)
{
	return (nvram_match("broadstream_iqos_default_conf", "1"));
}

/* Set nvram to apply default iQos configuration file or not
 *
 * @param       enable  1|0
 * @return      0 if success, -1 if fail.
 */
int
iqos_set_default_conf(int enable)
{
	if (nvram_match("broadstream_iqos_default_conf", "1")) {
		if (enable == 0) {
			nvram_set("broadstream_iqos_default_conf", "0");
		} else {
			return 0;
		}
	} else {
		if (enable == 0) {
			nvram_set("broadstream_iqos_default_conf", "0");
		} else {
			nvram_set("broadstream_iqos_default_conf", "1");
		}
	}

	nvram_commit();

	return 0;
}

/* Load default iQoS configuration file */
static int
iqos_load_default(void)
{
	eval("cp", PATH_SRC_FILE_QOS_CONF, PATH_QOS_CONF);
	iqos_file_backup();
	return 0;
}

/* Backup iQoS configuration file
 *
 * @return      0 if success, -1 if fail.
 */
int
iqos_file_backup(void)
{
#ifndef BCA_HNDROUTER
	if (mkdir(RAMFS_CONFMTD_DIR, 0777) < 0 && errno != EEXIST) {
		perror(RAMFS_CONFMTD_DIR);
		return -1;
	}
#endif /* !BCA_HNDROUTER */

	if (mkdir(CONFMTD_IQOS_DIR, 0777) < 0 && errno != EEXIST) {
		perror(CONFMTD_IQOS_DIR);
		return -1;
	}

	eval("cp", PATH_QOS_CONF, CONFMTD_IQOS_DIR);

#ifndef BCA_HNDROUTER
	return confmtd_backup();
#else
	return 0;
#endif
}

/* Restore iQos configuration file
 *
 * @return      0 if success, -1 if fail.
 */
static int
iqos_file_restore(void)
{
	char *conf_file = PATH_CONFMTD_QOS_CONF;
	struct stat tmp_stat;

	if (stat(conf_file, &tmp_stat) == 0) {
		eval("cp", conf_file, PATH_QOS_CONF);
		return 0;
	}

	return -1;
}

/* Split a string into tokens based on delimiters. The new tokens are malloced and returned
 * in the *tokenlist. Caller must free it by calling iqos_free_token_list.
 * Returns number of tokens found
 */
static int
iqos_split_string(char *str, char *delim, struct list_head *tokenlist)
{
	char *token = NULL;
	char *saveptr = NULL;
	iqos_tokens_t *newtoken;
	int count = 0;

	token = strtok_r(str, delim, &saveptr);
	while (token) {
		newtoken = (iqos_tokens_t *)malloc(sizeof(*newtoken));
		newtoken->value = strdup(token);
		list_add_tail(&(newtoken->list), tokenlist);
		token = strtok_r(NULL, delim, &saveptr);
		count++;
	}
	return count;
}

/* Free the iqos_tokens_t list members as well as the list */
static int
iqos_free_token_list(struct list_head *tokenlist)
{
	struct list_head *pos, *next;
	list_for_each_safe(pos, next, tokenlist) {
		iqos_tokens_t *token = list_entry(pos, iqos_tokens_t, list);
		free(token->value);
		list_del(&token->list);
		free(token);
	}
	return 0;
}

/* Print all tokens in a list */
static int
iqos_print_tokens(struct list_head *tokenlist, char *prefix)
{
	struct list_head *pos, *next;
	int count = 1;
	list_for_each_safe(pos, next, tokenlist) {
		iqos_tokens_t *token = list_entry(pos, iqos_tokens_t, list);
		IQOS_INFO("%s %d [%s]\n", prefix, count++, token->value);
	}
	return 0;
}

/* Validate the given URL. Return the pointer to the beginning of URL parameters */
static char *
iqos_validate_url(char *url)
{
	char *ptr;

	/* Verify that the URL is iqos.cgi itself */
	if (strncasecmp(url, "iqos.cgi?", strlen("iqos.cgi?")) != 0) {
		IQOS_INFO("URL received is not iqos.cgi. URL=[%s]\n", url);
		return NULL;
	}

	/* Skip the initial portion of the URL Ex: "/iqos.cgi?blahblah" */
	ptr = url + strlen("iqos.cgi?");
	return ptr;
}

/* Search for a given name in the tokenlist (name=value pair) and return the value portion */
static char *
iqos_get_value_from_list(struct list_head *tokenlist, char *name)
{
	struct list_head *pos, *next;
	list_for_each_safe(pos, next, tokenlist) {
		iqos_tokens_t *token = list_entry(pos, iqos_tokens_t, list);
		if (strncasecmp(token->value, name, strlen(name)) == 0) {
			return (token->value + strlen(name));
		}
	}
	return NULL;
}

/* To validate the parameters to include in output JSON object
	@param	tokenlist	device category ID
	@param	name		priority of device category
	@return	1 if success, 0 if fail
*/
static int
iqos_search_in_list(struct list_head *tokenlist, const char *name)
{
	if (list_empty(tokenlist)) {
		return 1;
	}

	struct list_head *pos, *next;
	list_for_each_safe(pos, next, tokenlist) {
		iqos_tokens_t *token = list_entry(pos, iqos_tokens_t, list);
		if (strcasecmp(token->value, name) == 0) {
			IQOS_INFO("%s Field is specified in List.\n", token->value);
			return 1;
		}
	}
	return 0;
}

/* Identify the request type from the "Req=" parameter of the URL */
static int
iqos_get_request_type(struct list_head *tokenlist)
{
	int reqtype = -1;
	int i;
	char *ptr = NULL;

	ptr = iqos_get_value_from_list(tokenlist, "Req=");
	if (ptr == NULL)
		return -1;

	/* Find out the corresponding enum value from giqos_req structure */
	for (i = 0; i < IQOS_UI_REQ_TYPE_MAX; i++) {
		if (strncasecmp(ptr, giqos_req[i].reqname, strlen(giqos_req[i].reqname)) == 0) {
			reqtype = giqos_req[i].reqid;
			break;
		}
	}
	IQOS_INFO("Request = [%s]\n", ptr);
	return reqtype;
}

/* Identify the Fields= parameeters and return back the list of parameters.
 * Caller must free fields_list
 */
static int
iqos_get_fields(struct list_head *nvlist, struct list_head *fields_list)
{
	char *fields;
	fields = iqos_get_value_from_list(nvlist, "Fields=");
	if (fields == NULL)
		return -1;
	iqos_split_string(fields, ",", fields_list);
	iqos_print_tokens(fields_list, "Fields");

	return 0;
}

/* Identify the MacAddr= parameters and return back the list of parameters.
 * Caller must free mac_address_list
 */
static int
iqos_get_mac_addrs(struct list_head *nvlist, struct list_head *mac_address_list)
{
	char *mac_addrs;
	mac_addrs = iqos_get_value_from_list(nvlist, "MacAddr=");
	if (mac_addrs == NULL)
		return -1;
	iqos_split_string(mac_addrs, ",", mac_address_list);
	iqos_print_tokens(mac_address_list, "MacAddresses");

	return 0;
}

/* Process the DeviceList Request */
static int
iqos_process_req_devicelist(struct list_head *nvlist)
{
	LIST_HEAD(fields_list);

	/* Extract the parameters */
	iqos_get_fields(nvlist, &fields_list);

	/* Start processing the request */
	g_iqos_output_buffer = (char *)iqos_get_connected_devices(&fields_list);

	/* Do cleanup */
	iqos_free_token_list(&fields_list);
	return 0;
}

/* Process the DeviceSummary Request */
static int
iqos_process_req_devicesummary(struct list_head *nvlist)
{
	LIST_HEAD(fields_list);
	LIST_HEAD(mac_addr_list);

	/* Extract the Mac Address List */
	iqos_get_mac_addrs(nvlist, &mac_addr_list);
	if (list_empty(&mac_addr_list)) {
		IQOS_ERROR("No Mac address list specified\n");
		return -1;
	}

	/* Extract the parameters */
	iqos_get_fields(nvlist, &fields_list);

	/* Start processing the request */
	g_iqos_output_buffer = (char *)iqos_get_device_details(&mac_addr_list, &fields_list);

	/* Do cleanup */
	iqos_free_token_list(&fields_list);
	iqos_free_token_list(&mac_addr_list);
	return 0;
}

/* Map the fields list to a flag mask. The *flags parameter will be populated with the list of
 * flags requested
 */
static int
iqos_map_fields(struct list_head *fields_list, unsigned long long *flags)
{
	struct list_head *pos, *next;
	int i = 0;
	list_for_each_safe(pos, next, fields_list) {
		i = 0;
		iqos_tokens_t *token = list_entry(pos, iqos_tokens_t, list);
		while (giqos_ui_fields[i].name[0] != '\0') {
			if (strcasecmp(token->value, giqos_ui_fields[i].name) == 0) {
				*flags |= giqos_ui_fields[i].mask;
				break;
			}
			i++;
		}
	}

	/* If no fields are requested it means all fields should be populated */
	if (i == 0) {
		*flags = ~0;
	}

	return 0;
}

/* This function gets the actual iqos settings and constructs a json object out of it */
static const char *
iqos_get_config(unsigned int flags)
{
	json_object *jobj;
	int upbw = 0, downbw = 0, refresh_interval = IQOS_MIN_REFRESH_RATE;
	const char *data = NULL;

	refresh_interval = atoi(nvram_safe_get("iqos_refresh_interval"));
	if ((flags & IQOS_FIELDS_CONFIG_DOWN_BW) || (flags & IQOS_FIELDS_CONFIG_UP_BW))
		iqos_get_wan_bw(&upbw, &downbw);

	jobj = json_object_new_object();
	json_object_object_add(jobj, "Req", json_object_new_string("GetConfig"));

	/* Check each flag and populate the fields only if that field is requested */
	if (flags & IQOS_FIELDS_CONFIG_ENABLE_IQOS)
		json_object_object_add(jobj, "EnableiQoS", json_object_new_int(iqos_is_enabled()));
	if (flags & IQOS_FIELDS_CONFIG_BW_AUTO)
		json_object_object_add(jobj, "BWAuto", json_object_new_int(iqos_is_wan_bw_auto()));
	if (flags & IQOS_FIELDS_CONFIG_DOWN_BW)
		json_object_object_add(jobj, "DownBW", json_object_new_int(downbw));
	if (flags & IQOS_FIELDS_CONFIG_UP_BW)
		json_object_object_add(jobj, "UpBW", json_object_new_int(upbw));

	/* Add refresh interval minimum will be 5 */
	json_object_object_add(jobj, "RefreshInterval",
		json_object_new_int(refresh_interval >= IQOS_MIN_REFRESH_RATE ?
		refresh_interval : IQOS_MIN_REFRESH_RATE));

	data = strdup(json_object_to_json_string(jobj));
	json_object_put(jobj);
	return data;
}

/* Process the GetConfig Request */
static int
iqos_process_req_getconfig(struct list_head *nvlist)
{
	unsigned long long flags = 0;
	LIST_HEAD(fields_list);

	/* Extract the parameters */
	iqos_get_fields(nvlist, &fields_list);
	iqos_map_fields(&fields_list, &flags);

	/* Start processing the request */
	g_iqos_output_buffer = (char *)iqos_get_config(flags);

	/* Do cleanup */
	iqos_free_token_list(&fields_list);

	return 0;
}

/* Create a json status response based on the status code */
static const char *
iqos_create_status_response(char *req, int status)
{
	json_object *jobj;
	const char *data = NULL;
	char *failreason = NULL;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "Req", json_object_new_string(req));

	switch (status) {
		case 0 :
			failreason = "";
			break;

		case iqos_Err_No_Input_Stream :
			failreason = "No data given for processing";
			break;

		case iqos_Err_Wrong_Request :
			failreason = "Invalid Request Type";
			break;

		case iqos_Err_Malformed_Request :
			failreason = "Malformed data given for processing";
			break;

		default :
			failreason = "Unknown Error";
			break;
	}
	if (status == 0) {
		json_object_object_add(jobj, "Status", json_object_new_string("SUCCESS"));
	} else {
		json_object_object_add(jobj, "Status", json_object_new_string("FAILURE"));
		json_object_object_add(jobj, "FailReason", json_object_new_string(failreason));
	}

	data = strdup(json_object_to_json_string(jobj));
	json_object_put(jobj);
	return data;
}

/* Read out the post data from the input stream and set the actual configurations */
static const char *
iqos_set_config(struct list_head *nvlist, FILE *stream, int len, const char *boundary)
{
	struct json_object *jobj, *jtempobj;
	int upbw = 0, downbw = 0;
	int enable = 0, bwauto = 0;
	int ret;
	int bwchange = 0;

	ret = iqos_verify_post_data(stream, len, boundary, &jobj, "SetConfig");
	if (ret != 0)
		return iqos_create_status_response("SetConfig", ret);

	jtempobj = json_object_object_get(jobj, "BWAuto");
	if (jtempobj) {
		bwauto = json_object_get_int(jtempobj);
		iqos_set_wan_bw_auto(bwauto);

		/* Setting manual upload and download bandwidth makes sense only if automatic
		 * bandwidth detection is not enabled.
		 */
		if (bwauto == 0) {
			jtempobj = json_object_object_get(jobj, "UpBW");
			upbw = json_object_get_int(jtempobj);
			jtempobj = json_object_object_get(jobj, "DownBW");
			downbw = json_object_get_int(jtempobj);
			iqos_set_wan_bw(upbw, downbw);
		}
		bwchange = 1;
	}

	jtempobj = json_object_object_get(jobj, "EnableiQoS");

	if (jtempobj) {
		enable = json_object_get_int(jtempobj);

		if (bwchange && enable)
			g_iqos_restart_flag |= IQOS_RESTART;

		if (!enable) {
			if (iqos_is_enabled()) {
				g_iqos_restart_flag |= IQOS_STOP;
			}
		} else {
			if (!iqos_is_enabled()) {
				g_iqos_restart_flag |= IQOS_START;
			}
		}
	}

	json_object_put(jobj);
	return iqos_create_status_response("SetConfig", 0);
}

/* Update the dev type prio from json obj */
static void
iqos_update_devtypeprio(struct json_object *jobj)
{
	int i, priority;
	char devtypeprio[16];
	struct json_object *jtempobj;

	/* data is in the form DevTypePrio0:1, DevTypePrio3:4 etc. Let us check which all
	 * priorities are sent from the UI
	 */
	for (i = 0; i < IQOS_MAX_DEVICES; i++) {
		snprintf(devtypeprio, sizeof(devtypeprio), "DevTypePrio%d", i);
		jtempobj = json_object_object_get(jobj, devtypeprio);
		if (jtempobj) {
			priority = json_object_get_int(jtempobj);

			/* Check if the priority is different from the current priority */
			if (priority != iqos_get_devs_priority(i)) {

				/* Ensure priority is in valid range */
				if ((priority >= 0) && (priority <= IQOS_MAX_PRIORITY)) {
					if (iqos_set_devs_priority(i, priority) == 0) {
						/* Restart the daemon if some value is changed */
						g_iqos_restart_flag |= IQOS_RESTART;
					}
				}
			}
		}
	}

}

/* Read out the post data from the input stream and set the actual device type priorities */
static const char *
iqos_set_devtypeprio(struct list_head *nvlist, FILE *stream, int len, const char *boundary)
{
	int ret;
	struct json_object *jobj;

	ret = iqos_verify_post_data(stream, len, boundary, &jobj, "SetDevTypePrio");
	if (ret != 0)
		return iqos_create_status_response("SetDevTypePrio", ret);
	iqos_update_devtypeprio(jobj);
	json_object_put(jobj);
	return iqos_create_status_response("SetDevTypePrio", 0);
}

/* Verify that the data posted using the HTTP post is valid data */
static int
iqos_verify_post_data(FILE *stream, int len, const char *boundary, struct json_object **jobj,
	char *expectedreq)
{
	char post_buf[2048];
	struct json_object *jtempobj;
	const char *req;

	if (!fgets(post_buf, MIN(len + 1, sizeof(post_buf)), stream))
		return iqos_Err_No_Input_Stream;

	*jobj = json_tokener_parse(post_buf);
	if (*jobj == NULL) {
		IQOS_ERROR("Malformed data given for %s\n", expectedreq);
		return iqos_Err_Malformed_Request;
	}

	/* Ensure it is a SetDevTypePrio request */
	jtempobj = json_object_object_get(*jobj, "Req");
	if (jtempobj == NULL) {
		IQOS_ERROR("Method Called without Req=%s\n", expectedreq);
		return iqos_Err_Wrong_Request;
	}
	req = json_object_get_string(jtempobj);
	if (strncasecmp(req, expectedreq, strlen(expectedreq) != 0)) {
		IQOS_ERROR("Method Called without Req=%s\n", expectedreq);
		return iqos_Err_Wrong_Request;
	}

	return 0;
}

/* Process the SetConfig Request */
static int
iqos_process_req_setconfig(struct list_head *nvlist, FILE *stream, int len, const char *boundary)
{
	g_iqos_output_buffer = (char *)iqos_set_config(nvlist, stream, len, boundary);

	return 0;
}

/* Search in a json array for a string value
 * name: Name of the json object
 * value : value to compare
 * Example : find in an array where 'devtype' == 'tablet'
 *           Caller has to call with name as 'devtype' and value as 'tablet'
 * return 1 if a match is found 0 otherwise.
 */
static int
search_in_json_array_string(struct json_object *jarray, char *name, char *value)
{
	int arraylen, i;
	const char *objval;
	struct json_object *jobj;

	arraylen = json_object_array_length(jarray);
	for (i = 0; i < arraylen; i++) {
		jobj = json_object_array_get_idx(jarray, i);
		if (jobj == NULL)
			return 0;

		/* Get the object that matches the name */
		objval = json_object_get_string(json_object_object_get(jobj, name));

		/* See if it's value is what is requested */
		if (strcasecmp(objval, value) == 0) {
			return 1;
		}
	}
	return 0;
}

/* Search in a json array for an int value
 * name: Name of the json object
 * value : value to compare
 * Example : find in an array where 'typeid' == '32'
 *           Caller has to call with name as 'typeid' and value as '32'
 * return 1 if a match is found 0 otherwise.
 */
static int
search_in_json_array_int(struct json_object *jarray, char *name, int value)
{
	int arraylen, i, objval;
	struct json_object *jobj;

	arraylen = json_object_array_length(jarray);
	for (i = 0; i < arraylen; i++) {
		jobj = json_object_array_get_idx(jarray, i);

		/* Get the object that matches the name */
		objval = json_object_get_int(json_object_object_get(jobj, name));

		/* See if it's value is what is requested */
		if (objval == value) {
			return 1;
		}
	}
	return 0;
}

/* Gets the actual iqos device type priorities and constructs a json object out of it */
static const char *
iqos_get_devtypeprio(struct list_head *devtypes_list)
{
	json_object *jobj, *jarrayobj, *jarraylist;
	int ret, i = 0, prio, typeid;
	const char *data = NULL;
	struct list_head *pos, *next;
	dev_os_t *os;
	char typename[IQOS_BUFF_SIZE];
	LIST_HEAD(dev_os_head);

	/* Do all initialization */
	init_dev_os(&dev_os_head);
	jobj = json_object_new_object();
	jarraylist = json_object_new_array();
	json_object_object_add(jobj, "Req", json_object_new_string("GetDevTypePrio"));

	/* First check if a list of devtypes is requested */
	list_for_each_safe(pos, next, devtypes_list) {
		iqos_tokens_t *token = list_entry(pos, iqos_tokens_t, list);
		typeid = atoi(token->value);
		typename[0] = '\0';

		prio = iqos_get_devs_priority(typeid);
		/* Get the type_name corresponding to the typeid */
		ret = iqos_get_dev_os_typename(&dev_os_head, typeid, typename, IQOS_BUFF_SIZE);
		if (ret < 0) {
			IQOS_WARNING("No matching Device type found for id [%d]\n", typeid);
			i++;
			continue;
		}
		jarrayobj = json_object_new_object();
		json_object_object_add(jarrayobj, "DevType", json_object_new_string(typename));
		json_object_object_add(jarrayobj, "DevTypeInt", json_object_new_int(typeid));
		json_object_object_add(jarrayobj, "Priority", json_object_new_int(prio));
		json_object_array_add(jarraylist, jarrayobj);
		i++;
	}

	if (i == 0) {
		/* List of devtypes is not requested. Give all availale devtype priorities. */

		/* Get the list of all available devices */
		list_for_each_safe(pos, next, &dev_os_head) {
			os = list_entry(pos, dev_os_t, list);
			prio = iqos_get_devs_priority(os->type_id);
			if (prio < 0) {
				IQOS_WARNING("No matching priority found for id [%d]\n",
					os->type_id);
				continue;
			}
			typename[0] = '\0';
			/* Get the type_name corresponding to the typeid */
			ret = iqos_get_dev_os_typename(&dev_os_head, os->type_id,
				typename, IQOS_BUFF_SIZE);
			if (ret < 0) {
				IQOS_WARNING("No matching device type found for id [%d]\n",
					os->type_id);
				continue;
			}

			/* Check if this devicetype is already added in the list. If it is already
			 * added, then don't add it to the list
			 */
			if (search_in_json_array_int(jarraylist, "DevTypeInt", os->type_id)) {
				continue;
			}
			jarrayobj = json_object_new_object();
			json_object_object_add(jarrayobj, "DevType",
				json_object_new_string(typename));
			json_object_object_add(jarrayobj, "DevTypeInt",
				json_object_new_int(os->type_id));
			json_object_object_add(jarrayobj, "Priority", json_object_new_int(prio));
			json_object_array_add(jarraylist, jarrayobj);
		}
	}
	free_dev_os(&dev_os_head);
	json_object_object_add(jobj, "DevTypePrio", jarraylist);

	data = strdup(json_object_to_json_string(jobj));

	json_object_put(jobj);

	return data;
}

/* Process the GetDevTypePrio Request */
static int
iqos_process_req_getdevtypeprio(struct list_head *nvlist)
{
	char *devtypes;
	LIST_HEAD(devtypes_list);

	/* Extract the devtype parameters */
	devtypes = iqos_get_value_from_list(nvlist, "DevType=");
	if (devtypes)
		iqos_split_string(devtypes, ",", &devtypes_list);

	/* Start processing the request */
	g_iqos_output_buffer = (char *)iqos_get_devtypeprio(&devtypes_list);

	/* Do cleanup */
	iqos_free_token_list(&devtypes_list);

	return 0;
}

/* Process the SetDevTypePrio Request */
static int
iqos_process_req_setdevtypeprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary)
{
	g_iqos_output_buffer = (char *)iqos_set_devtypeprio(nvlist, stream, len, boundary);
	return 0;
}

/* Gets the actual iqos application category priorities and constructs a json object out of it */
static const char *
iqos_get_appcatprio(struct list_head *appcats_list)
{
	json_object *jobj, *jarrayobj, *jarraylist;
	int i = 0, prio, catid;
	const char *data = NULL;
	char *catname = NULL;
	struct list_head *pos, *next;
	app_cat_t* cat;
	LIST_HEAD(app_cat_head);

	/* Do all initialization */
	init_app_cat(&app_cat_head);
	jobj = json_object_new_object();
	jarraylist = json_object_new_array();
	json_object_object_add(jobj, "Req", json_object_new_string("GetAppCatPrio"));

	/* First check if a list of appcats is requested */
	list_for_each_safe(pos, next, appcats_list) {
		iqos_tokens_t *token = list_entry(pos, iqos_tokens_t, list);
		catid = atoi(token->value);

		prio = iqos_get_apps_priority(catid);
		/* Get the cat_name corresponding to the catid */
		catname = search_app_cat(&app_cat_head, catid);
		if (catname == NULL) {
			IQOS_WARNING("No matching Application Category found for id [%d]\n", catid);
			i++;
			continue;
		}
		jarrayobj = json_object_new_object();
		json_object_object_add(jarrayobj, "Category", json_object_new_string(catname));
		json_object_object_add(jarrayobj, "CategoryInt", json_object_new_int(catid));
		json_object_object_add(jarrayobj, "Priority", json_object_new_int(prio));
		json_object_array_add(jarraylist, jarrayobj);
		i++;
	}

	if (i == 0) {
		/* List of appcats is not requested. Give all availale appcat priorities. */

		/* Scan through the list of all app categories */
		list_for_each_safe(pos, next, &app_cat_head) {
			cat = list_entry(pos, app_cat_t, list);
			prio = iqos_get_apps_priority(cat->cat_id);
			jarrayobj = json_object_new_object();
			json_object_object_add(jarrayobj, "Category",
				json_object_new_string(cat->cat_name));
			json_object_object_add(jarrayobj, "CategoryInt",
				json_object_new_int(cat->cat_id));
			json_object_object_add(jarrayobj, "Priority", json_object_new_int(prio));
			json_object_array_add(jarraylist, jarrayobj);
		}
	}
	free_app_cat(&app_cat_head);
	json_object_object_add(jobj, "AppCatPrio", jarraylist);

	data = strdup(json_object_to_json_string(jobj));

	json_object_put(jobj);

	return data;
}

/* Process the GetAppCatPrio Request */
static int
iqos_process_req_getappcatprio(struct list_head *nvlist)
{
	char *appcats;
	LIST_HEAD(appcats_list);

	/* Extract the devtype parameters */
	appcats = iqos_get_value_from_list(nvlist, "AppCat=");
	if (appcats)
		iqos_split_string(appcats, ",", &appcats_list);

	/* Start processing the request */
	g_iqos_output_buffer = (char *)iqos_get_appcatprio(&appcats_list);

	/* Do cleanup */
	iqos_free_token_list(&appcats_list);

	return 0;
}

/* Update the application category prio from json obj */
static void
iqos_update_appcatprio(struct json_object *jobj)
{
	int i, priority;
	char appcatprio[16];
	struct json_object *jtempobj;

	/* data is in the form AppCatPrio0:1, AppCatPrio1:4 etc. Let us check which all
	 * priorities are sent from the UI
	 */
	for (i = 0; i < IQOS_MAX_CAT_NODE; i++) {
		snprintf(appcatprio, sizeof(appcatprio), "AppCatPrio%d", i);
		jtempobj = json_object_object_get(jobj, appcatprio);
		if (jtempobj) {
			priority = json_object_get_int(jtempobj);

			/* Check if the priority is different from the current priority */
			if (priority != iqos_get_apps_priority(i)) {

				/* Ensure priority is in valid range */
				if ((priority >= 0) && (priority <= IQOS_MAX_PRIORITY)) {
					if (iqos_set_apps_priority(i, priority) == 0) {
						/* Restart the daemon if some value is changed */
						g_iqos_restart_flag |= IQOS_RESTART;
					}
				}
			}
		}
	}
}

/* Read out the post data from the input stream and set the actual appcat priorities */
static const char *
iqos_set_appcatprio(struct list_head *nvlist, FILE *stream, int len, const char *boundary)
{
	struct json_object *jobj;
	int ret;

	ret = iqos_verify_post_data(stream, len, boundary, &jobj, "SetAppCatPrio");
	if (ret != 0)
		return iqos_create_status_response("SetAppCatPrio", ret);

	iqos_update_appcatprio(jobj);

	json_object_put(jobj);
	return iqos_create_status_response("SetAppCatPrio", 0);
}

/* Process the SetAppCatPrio Request */
static int
iqos_process_req_setappcatprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary)
{
	g_iqos_output_buffer = (char *)iqos_set_appcatprio(nvlist, stream, len, boundary);
	return 0;
}

/* Process the GetMacPrio Request */
static int
iqos_process_req_getmacprio(struct list_head *nvlist)
{
	char *macaddrs;
	LIST_HEAD(macaddrs_list);

	/* Extract the devtype parameters */
	macaddrs = iqos_get_value_from_list(nvlist, "MacAddr=");
	if (macaddrs)
		iqos_split_string(macaddrs, ",", &macaddrs_list);

	/* Start processing the request */
	g_iqos_output_buffer = (char *)iqos_get_macprio(&macaddrs_list);

	/* Do cleanup */
	iqos_free_token_list(&macaddrs_list);
	return 0;
}

/* Updates the all prio(application/category/mac) settings */
static const char*
iqos_set_allprio(struct list_head *nvlist, FILE *stream, int len, const char *boundary)
{
	struct json_object *jobj;
	int ret;

	ret = iqos_verify_post_data(stream, len, boundary, &jobj, "SetAllPrio");
	if (ret != 0) {
		return iqos_create_status_response("SetAllPrio", ret);
	}

	iqos_update_appcatprio(jobj);
	iqos_update_devtypeprio(jobj);
	iqos_update_macprio(jobj);

	json_object_put(jobj);
	return iqos_create_status_response("SetAllPrio", 0);
}

/* Process SetAllPrio Request */
static int
iqos_process_req_setallprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary)
{
	g_iqos_output_buffer = (char*)iqos_set_allprio(nvlist, stream, len, boundary);
	return 0;
}

/* Restores the qos.conf file except wan bw settings */
static const char*
iqos_set_defaultprio(FILE *stream, int len, const char *boundary)
{
	struct json_object *jobj;
	int downbw = 0, upbw = 0, ret = 0;

	/* Verify the post stream */
	ret = iqos_verify_post_data(stream, len, boundary, &jobj, "SetDefaultPrio");
	if (ret != 0) {
		return iqos_create_status_response("SetDefaultPrio", ret);
	}

	/* Save the upload and download bandwidth */
	iqos_get_wan_bw(&upbw, &downbw);

	/* Load the default configuration file */
	iqos_load_default();

	/* Update the upload and download bandwidth */
	iqos_set_wan_bw(upbw, downbw);

	g_iqos_restart_flag |= IQOS_RESTART;

	json_object_put(jobj);
	return iqos_create_status_response("SetDefaultPrio", 0);
}

/* Process RestoreAllPrio Request */
static int
iqos_process_req_setdefaultprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary)
{
	g_iqos_output_buffer = (char*)iqos_set_defaultprio(stream, len, boundary);
	return 0;
}

/* Process the SetMacPrio Request */
static int
iqos_process_req_setmacprio(struct list_head *nvlist, FILE *stream, int len,
	const char *boundary)
{
	g_iqos_output_buffer = (char *)iqos_set_macprio(nvlist, stream, len, boundary);
	return 0;
}

/* Gets the actual iqos MAC based priorities and constructs a json object out of it */
static const char *
iqos_get_macprio(struct list_head *mac_list)
{
	json_object *jobj, *jarrayobj, *jarraylist;
	int prio, i = 0, ret;
	devid_user_e *usr_lst = NULL;
	const char *data = NULL;
	char *macaddr = NULL;
	char dev_mac[IQOS_MACSTR_LEN];
	struct list_head *pos, *next;
	char buffer[IQOS_BUFF_SIZE] = {'\0'};

	LIST_HEAD(dev_os_head);
	dev_os_t *dev_os;
	init_dev_os(&dev_os_head);

	/* Do all initialization */
	jobj = json_object_new_object();
	jarraylist = json_object_new_array();
	json_object_object_add(jobj, "Req", json_object_new_string("GetMacPrio"));

	/* Get list of devices */
	ret = GET_FW_USERLIST(&usr_lst);
	if (ret < 0) {
		if (usr_lst) {
			free(usr_lst);
			usr_lst = NULL;
		}
		goto err;
	}

	/* Run through the list of macaddresses */
	list_for_each_safe(pos, next, mac_list) {
		iqos_tokens_t *token = list_entry(pos, iqos_tokens_t, list);
		macaddr = strdup(token->value);

		/* MAC addresses may have %3A in it - HTML escape for ':' */
		iqos_inline_url_unescape(macaddr);
		prio = iqos_get_mac_priority_ex(macaddr);
		if (prio < 0) {
			free(macaddr);
			i++;
			continue;
		}
		jarrayobj = json_object_new_object();

		for (i = 0; i < DEVID_MAX_USER; i++) {

			if (usr_lst[i].available <= 0)
				break;

			snprintf(dev_mac, sizeof(dev_mac), MAC_OCTET_FMT,
				MAC_OCTET_EXPAND(usr_lst[i].mac));

			if (strncmp(dev_mac, macaddr, strlen(macaddr)) == 0) {
				json_object_object_add(jarrayobj, "Name",
					json_object_new_string(usr_lst[i].host_name));
				snprintf(buffer, sizeof(buffer), IPV4_OCTET_FMT,
					IPV4_OCTET_EXPAND(usr_lst[i].ipv4));
				json_object_object_add(jarrayobj, "IPAddr",
					json_object_new_string(buffer));
				json_object_object_add(jarrayobj, "DevTypeInt",
					json_object_new_int(USR_TYPE_ID(i)));
				break;
			}
		}

		json_object_object_add(jarrayobj, "MacAddr", json_object_new_string(macaddr));
		json_object_object_add(jarrayobj, "Priority", json_object_new_int(prio));
		json_object_array_add(jarraylist, jarrayobj);
		free(macaddr);
		i++;
	}

	/* No list of macaddresses passed. We will send out priorities of the following
	   1. List of available devices on network
	   2. Any device whose priority is previousely set (eventhough it is not in the n/w now
	 */

	if (i == 0) {
		iqos_mac_priority_t preset_devs[IQOS_MAX_DEVICES_WITH_MACPRIORITY];
		int num_mac_devs;

		for (i = 0; i < DEVID_MAX_USER; i++) {
			if (usr_lst[i].available <= 0)
				break;

			/* Find their priority based on mac address */
			snprintf(dev_mac, sizeof(dev_mac), MAC_OCTET_FMT,
				MAC_OCTET_EXPAND(usr_lst[i].mac));
			prio = iqos_get_mac_priority_ex(dev_mac);

			/* if proper priority is not received, let us continue to next one */
			if (prio < 0)
				continue;

			jarrayobj = json_object_new_object();

			json_object_object_add(jarrayobj, "Name",
				json_object_new_string(usr_lst[i].host_name));

#ifdef BCA_HNDROUTER
			dev_os = search_dev_os(&dev_os_head,
					usr_lst[i].os.de.vendor_id,
					usr_lst[i].os.de.name_id,
					usr_lst[i].os.de.class_id,
					usr_lst[i].os.de.cat_id,
					usr_lst[i].os.de.dev_id,
					usr_lst[i].os.de.family_id);
#else /* BCA_HNDROUTER */
			dev_os = search_dev_os(&dev_os_head,
				usr_lst[i].os.detail.vendor_id,
				usr_lst[i].os.detail.os_id,
				usr_lst[i].os.detail.class_id,
				usr_lst[i].os.detail.type_id,
				usr_lst[i].os.detail.dev_id,
				usr_lst[i].os.detail.family_id);
#endif /* !BCA_HNDROUTER */

			if (dev_os != NULL) {
				if (dev_os->dev_name != NULL)
					json_object_object_add(jarrayobj, "DevName",
						json_object_new_string(dev_os->dev_name));
			}

			snprintf(buffer, sizeof(buffer), IPV4_OCTET_FMT,
				IPV4_OCTET_EXPAND(usr_lst[i].ipv4));
			json_object_object_add(jarrayobj, "IPAddr",
				json_object_new_string(buffer));
			json_object_object_add(jarrayobj, "DevTypeInt",
				json_object_new_int(USR_TYPE_ID(i)));

			json_object_object_add(jarrayobj, "MacAddr",
				json_object_new_string(dev_mac));
			json_object_object_add(jarrayobj, "Priority", json_object_new_int(prio));
			json_object_array_add(jarraylist, jarrayobj);
		}

		/* Get the list of devices which has priority assigned earlier */
		num_mac_devs = iqos_list_mac_priority(preset_devs,
			IQOS_MAX_DEVICES_WITH_MACPRIORITY);
		for (i = 0; i < num_mac_devs; i++) {
			/* If the device is already present in the output list, don't add it */
			if (search_in_json_array_string(jarraylist, "MacAddr",
				preset_devs[i].macaddr))
				continue;
			jarrayobj = json_object_new_object();
			json_object_object_add(jarrayobj, "MacAddr",
				json_object_new_string(preset_devs[i]. macaddr));
			json_object_object_add(jarrayobj, "Priority",
				json_object_new_int(preset_devs[i].prio));
			json_object_array_add(jarraylist, jarrayobj);
		}

	}

err:
	json_object_object_add(jobj, "MacPrio", jarraylist);
	data = strdup(json_object_to_json_string(jobj));
	json_object_put(jobj);
	free_dev_os(&dev_os_head);
	if (usr_lst)
		free(usr_lst);

	return data;
}

/* Update the device mac prio from json obj */
static void
iqos_update_macprio(struct json_object *jobj)
{
	int i, priority;
	char macaddr[16];
	char macaddr_prio[16];
	char *macaddr_set;
	struct json_object *jobjmacaddr, *jobjmacaddr_prio;

	/* data is in the form MacAddr1:0A:0B:0C:0D:0E:0F, MacAddrPrio1:2,
	 * MacAddr2:01:12:23:34:45:56, MacAddrPrio2:1,
	 * Keep searching for MacAddr1, MacAddr2 etc till we hit failure.
	 * If we hit failure stop looking further
	 */
	for (i = 0; i < IQOS_MAX_DEVICES; i++) {
		snprintf(macaddr, sizeof(macaddr), "MacAddr%d", i);
		snprintf(macaddr_prio, sizeof(macaddr_prio), "MacAddrPrio%d", i);
		jobjmacaddr = json_object_object_get(jobj, macaddr);
		jobjmacaddr_prio = json_object_object_get(jobj, macaddr_prio);
		if (jobjmacaddr && jobjmacaddr_prio) {
			macaddr_set = (char *)json_object_get_string(jobjmacaddr);
			priority = json_object_get_int(jobjmacaddr_prio);

			/* Check if the priority is different from the current priority */
			if (priority != iqos_get_mac_priority_ex(macaddr_set)) {

				/* Ensure priority is in valid range */
				if ((priority >= 0) && (priority <= IQOS_MAX_PRIORITY)) {
					if (iqos_set_mac_priority(macaddr_set, priority) == 0) {
						/* Restart the daemon if some value is changed */
						g_iqos_restart_flag |= IQOS_RESTART;
					}
				}
			}
		} else {
			/* No such MacAddr object present in the request. Stop looking further */
			break;
		}
	}
}

/* Read out the post data from the input stream and set the actual MAC based priorities */
static const char *
iqos_set_macprio(struct list_head *nvlist, FILE *stream, int len, const char *boundary)
{
	struct json_object *jobj;
	int ret;

	ret = iqos_verify_post_data(stream, len, boundary, &jobj, "SetMacPrio");
	if (ret != 0)
		return iqos_create_status_response("SetMacPrio", ret);

	iqos_update_macprio(jobj);

	json_object_put(jobj);
	return iqos_create_status_response("SetMacPrio", 0);
}

#endif /* __CONFIG_TREND_IQOS__ */
