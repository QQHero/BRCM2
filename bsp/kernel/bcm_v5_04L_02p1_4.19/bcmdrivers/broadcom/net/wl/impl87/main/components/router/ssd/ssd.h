/*
 * SSD shared include file
 *
 * Copyright (C) 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * $Id: ssd.h $
 */

#define NVRAM_SSD_ENABLE "ssd_enable"
#define NVRAM_SSD_SSID_TYPE "ssd_type"

#define BCM_EVENT_HEADER_LEN	(sizeof(bcm_event_t))
#define MAX_EVENT_BUFFER_LEN	1400
#define MAX_LINE_LEN			16

/* wait for security check */
#define INTERVAL_ASSOC_CONFIRM 5
#define DISCONNECT_EVENT(e) ((e == WLC_E_DISASSOC_IND) ||\
							(e == WLC_E_DEAUTH) ||\
							(e == WLC_E_DEAUTH_IND))

typedef enum {
	SSD_TYPE_DISABLE = 0,
	SSD_TYPE_PRIVATE = 1,
	SSD_TYPE_PUBLIC = 2
} ssd_type_t;

typedef struct ssd_maclist {
	struct ether_addr addr;
	time_t timestamp;		/* assoc timestamp */
	uint8  ifidx;			/* destination OS i/f index */
	uint8  bsscfgidx;		/* source bsscfg index */
	char   ssid[MAX_SSID_LEN + 1];
	uint8  security;		/* open, WEP or WPA/WPA2 */
	uint8  softblocked;		/* flag after adding to deny list on public interface */
	struct ssd_maclist *next;
} ssd_maclist_t;

#define SSD_DEBUG_ERROR		0x000001
#define SSD_DEBUG_WARNING	0x000002
#define SSD_DEBUG_INFO		0x000004

#define SSD_PRINT_ERROR(fmt, arg...) \
		do { if (ssd_msglevel & SSD_DEBUG_ERROR) \
			printf("SSD >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); } while (0)

#define SSD_PRINT_WARNING(fmt, arg...) \
		do { if (ssd_msglevel & SSD_DEBUG_WARNING) \
			printf("SSD >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); } while (0)

#define SSD_PRINT_INFO(fmt, arg...) \
		do { if (ssd_msglevel & SSD_DEBUG_INFO) \
			printf("SSD >>%s(%d): "fmt, __FUNCTION__, __LINE__, ##arg); } while (0)

/* enhancement for softblock clear/list */
#define SSD_OK				0
#define SSD_FAIL			-1

#define SSD_SOFTBLOCK_LIST_FILE		"/tmp/ssd_softblock_list.log"
#define SSD_SOFTBLOCK_LIST_FILE_TMP	"/tmp/ssd_softblock_list.log.tmp"

#define NVRAM_SSD_DEBUG_LEVEL		"ssd_msglevel"

/* cli to daemon socket */
#define SSD_DEFAULT_FD			-1
#define EAPD_WKSP_SSD_CLI_PORT		EAPD_WKSP_SSD_UDP_PORT + 65
#define SSD_DEFAULT_SERVER_HOST		"127.0.0.1"

typedef enum {
	SSD_CMD_SET_MSGLEVEL = 0,
	SSD_CMD_SOFTBLOCK_CLEAR = 1,
	SSD_CMD_SOFTBLOCK_LIST = 2
} ssd_cmd_id_t;

static int ssd_daemon_proc_cli_req(void);
