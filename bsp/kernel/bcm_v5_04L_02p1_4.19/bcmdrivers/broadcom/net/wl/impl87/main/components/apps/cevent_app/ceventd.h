/*
 * Cevent daemon header
 *
 *
 * Copyright (C) 2022, Broadcom. All Rights Reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 *
 * $Id: ceventd.h 809956 2022-03-28 14:37:00Z $
 */

#ifndef __CEVENTD_H__
#define __CEVENTD_H__

#include "cevent_app_common.h"
#include "bcmutils.h"

/* enable structure packing */
#include <packed_section_start.h>

/* default verbosity */
extern CA_VERBOSE ca_d_verbose;

#define CA_OUT_FLUSH_FACTOR		4U	/* fflush for every these many log entries */
#define CA_OUT_HEAD_FACTOR		50U	/* print key heading for every log entries */
#define CA_OUT_LIMIT_MIN_KB		1U	/* min valid number for nvram ceventd_out_limit */
#define CA_OUT_LIMIT_MAX_KB		1024U	/* max valid number for nvram ceventd_out_limit */
#define CA_OUT_LIMIT_DEFAULT_KB		64U	/* default size of log file in kB */
#define CA_PKTLOG_OUT_LIMIT_DEFAULT_KB	64U	/* default size of log file in kB */
#define CA_PKTLOG_DUMP_PERIOD		1000U	/* 1000ms = 1sec */

#define CA_WKSP_FLAG_SHUTDOWN		(1U << 1)
#define CA_WKSP_FLAG_USR1		(1U << 2)
#define CA_WKSP_FLAG_PAUSE		(1U << 3)

#define CA_PKT_LEN			4096U	/* cevent eapd socket packet limit */
#define CA_CLI_REQ_LEN			1024U	/* cevent cli socket request limit */

#define PKTLOG_PCAP_MAGIC_NUM		0xa1b2c3d4U	/* pcap magic number  */
#define PKTLOG_PCAP_MAJOR_VER		0x02U		/* pcap major version */
#define PKTLOG_PCAP_MINOR_VER		0x04U		/* pcap mintor version */
#define PKTLOG_PCAP_SNAP_LEN		0x40000U	/* max snapshot length */
#define PKTLOG_PCAP_NETWORK_TYPE	105U		/* network type = 105 (802.11) */

#if CA_CLI_REQ_LEN > CA_CLI_RSP_LEN
#error "CA_CLI_REQ_LEN must be less than or equal to CA_CLI_RSP_LEN"
#endif /* CA_CLI_REQ_LEN > CA_CLI_RSP_LEN */

#define CA_IS_EAPOL_OR_PREAUTH(et)							\
	((((et) == ETHER_TYPE_802_1X) || ((et) == ETHER_TYPE_802_1X_PREAUTH)) ?		\
	TRUE : FALSE)

#define CA_WATCHDOG_PERIOD_SEC	1	/* run watchdog every these many seconds */

/* Prints with CEVENTD__<verb> and function name banner
 * where <verb> could be any suffix but typically verbosity suffix like "ERR", "MSG", "DBG"
 */
#define CA_PRT(verb, fmt, arg...) \
	printf("CEVENTD_" verb "> %s(): "fmt, __FUNCTION__, ##arg)

#if defined(BCMDBG)
#define CA_ERR(fmt, arg...) CA_PRT("ERR", fmt, ##arg)
#else /* BCMINTERNAL || BCMDBG */
#define CA_ERR(fmt, arg...) if (ca_d_verbose >= CA_VERBOSE_ERR) CA_PRT("ERR", fmt, ##arg)
#endif

#define CA_MSG(fmt, arg...) if (ca_d_verbose >= CA_VERBOSE_MSG) CA_PRT("MSG", fmt, ##arg)
#define CA_DBG(fmt, arg...) if (ca_d_verbose >= CA_VERBOSE_DBG) CA_PRT("DBG", fmt, ##arg)
/* debug log without banner */
#define CA_DBG_MIN(fmt, arg...) if (ca_d_verbose >= CA_VERBOSE_DBG) printf(fmt, ##arg)

#define CA_LOG_TYPE_SSV	0 /* space separated values with header per CA_OUT_HEAD_FACTOR */
#define CA_LOG_TYPE_CSV	1 /* comma separated values with header per CA_OUT_HEAD_FACTOR */

/*
 * Wireshark compatible file(pcap) format description
 * Refer to https://wiki.wireshark.org/Development/LibpcapFileFormat#global-header
 * | Global Header(0ffset 0) | Packet Header | Packet Data | Packet Header | Packet Data | ...
 */

/* Global header */
typedef struct ca_pktlog_pcap_hdr {
	uint32 magic_number;			/* magic number */
	uint16 version_major;			/* major version number */
	uint16 version_minor;			/* minor version number */
	uint32 thiszone;			/* GMT to local correction */
	uint32 sigfigs;				/* accuracy of timestamps */
	uint32 snaplen;				/* max length of captured packets, in octets */
	uint32 network;				/* data link type */
} ca_pktlog_pcap_hdr_t;

/* Record(packet) header */
typedef struct ca_pktlog_pcap_rec_hdr {
	uint32 ts_sec;				/* timestamp seconds */
	uint32 ts_usec;				/* timestamp microseconds */
	uint32 incl_len;			/* number of octets of packet saved in file */
	uint32 orig_len;			/* actual length of packet */
} ca_pktlog_pcap_rec_hdr_t;

typedef struct pktlog_node {
	dll_t pktlog_list_p;			/* pktlog list pointer */
	ca_pktlog_pcap_rec_hdr_t pcap_rec_h;	/* data length from CEVENT */
	void *data;				/* data from CEVENT */
} pktlog_node_t;

typedef struct {
	/* *** NOTE: Keep pkt_* elements at top to retain 4 byte boundary *** */
	uint8 pkt_eapd[CA_PKT_LEN];		/* buffer for eapd socket send/recv */
	uint8 pkt_cli_req[CA_CLI_REQ_LEN];	/* buffer for cli socket to receive request */
	uint8 pkt_cli_rsp[CA_CLI_RSP_LEN];	/* buffer for cli socket to send response */

	int pid;				/* current process id */
	int pid_fd;				/* FD to lock for this process ID */
	uint32 flags;				/* see CA_WKSP_FLAG_* defines for possible values */
	char interface[IFNAMSIZ+1];		/* LAN interface name */
	uint32	log_type;			/* see CA_LOG_TYPE_* */
	char out_path[CA_FILE_PATH_LEN];	/* output file path */
	char out_bak_path[CA_FILE_PATH_LEN];	/* output backup file path */
	char pktlog_out_path[CA_FILE_PATH_LEN];	/* output file path */
	char pktlog_out_bak_path[CA_FILE_PATH_LEN];	/* output backup file path */
	FILE *out;				/* event log output stream; defaults to stdout */
	FILE *pktlog_out;			/* pktlog output stream */
	dll_t pktlog_h;				/* pktlog list header */
	uint32 pktlog_out_size;			/* pktlog output file size */
	uint32 pktlog_dump_s;			/* previous dump size */
	uint64 pktlog_dump_ts;			/* pktlog previous dump time stamp */
	bool   pktlog_dump_triggered;		/* pktlog dump triggered */
	uint32 pktlog_dump_period;		/* pktlog dump time period */
	uint32 pktlog_out_limit;		/* pktlog file size limit */
	uint32 out_size;			/* estimated size of output file */
	uint32 out_limit;			/* size limit of output file in bytes */
	uint32 num_logs;			/* number of log entries written so far */
	uint32 log_console;			/* log output to console */
	uint32 log_syslogd;			/* send output to syslogd */
	uint32 tick;				/* tick seconds */
	uint64 recent_event_ts;			/* timestamp of recent most event */
	uint64 watchdog_ts;			/* timestamp when watchdog ran */
	int eapd_fd;				/* socket FD to get/send bcmevent from/to eapd */
	int cli_fd;				/* server socket FD to listen from CLI */
} ca_wksp_t;

/* WPS Message types */
#define CA_WSC_START		0x01U
#define CA_WSC_ACK		0x02U
#define CA_WSC_NACK		0x03U
#define CA_WSC_MSG		0x04U
#define CA_WSC_DONE		0x05U
#define CA_WSC_FRAG_ACK		0x06U

#define CA_WPS_ID_MESSAGE_M1	0x04U
#define CA_WPS_ID_MESSAGE_M2	0x05U
#define CA_WPS_ID_MESSAGE_M2D	0x06U
#define CA_WPS_ID_MESSAGE_M3	0x07U
#define CA_WPS_ID_MESSAGE_M4	0x08U
#define CA_WPS_ID_MESSAGE_M5	0x09U
#define CA_WPS_ID_MESSAGE_M6	0x0AU
#define CA_WPS_ID_MESSAGE_M7	0x0BU
#define CA_WPS_ID_MESSAGE_M8	0x0CU
#define CA_WPS_ID_MESSAGE_ACK	0x0DU
#define CA_WPS_ID_MESSAGE_NACK	0x0EU
#define CA_WPS_ID_MESSAGE_DONE	0x0FU

#define CA_WPS_MSGTYPE_OFFSET	9U
#define CA_VENDOR_ID_SZ		3U

typedef BWL_PRE_PACKED_STRUCT struct ca_wps_eap_header_tag {
	uint8 code;
	uint8 id;
	uint16 length;
	uint8 type;
	uint8 vendorId[CA_VENDOR_ID_SZ];
	uint32 vendorType;
	uint8 opcode;
	uint8 flags;
} BWL_POST_PACKED_STRUCT ca_wps_eap_hdr;

extern char *ca_bcm_event_str[WLC_E_LAST + 1]; /* array is auto-generated by gen_be_str.sh */

extern int
ca_out_file_init(ca_wksp_t *cwksp);

extern int
ca_out_file_reinit(ca_wksp_t *cwksp);

extern int
ca_out_file_flush(ca_wksp_t *cwksp);

extern int
ca_cli_init(ca_wksp_t *cwksp);

extern int
ca_cli_deinit(ca_wksp_t *cwksp);

extern int
ca_open_eapd(ca_wksp_t *cwksp);

extern void
ca_close_eapd(ca_wksp_t *cwksp);

extern int
ca_eapd_send_pkt(ca_wksp_t *cwksp, struct iovec *frags, int nfrags);

extern int
ca_socket_process(ca_wksp_t *cwksp);

extern void
ca_pktlog_init(ca_wksp_t *cwksp);

extern void
ca_pktlog_reinit(ca_wksp_t *cwksp);

extern void
ca_pktlog_deinit(ca_wksp_t *cwksp);

extern void
ca_write_pktlog(ca_wksp_t *cwksp);

extern void
ca_pktlog_add_pcap_header(ca_wksp_t *cwksp);

extern void
ca_config_pktlog_init_out_file(ca_wksp_t *cwksp, const char *nvram_val);
#endif /* __CEVENTD_H__ */
