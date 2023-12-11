/*
 * Broadcom Wifi Data Model Library.
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
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <sys/file.h>
#include <limits.h>
#include <ctype.h>

#include <typedefs.h>
#include "wlcsm_defs.h"
#include "wlcsm_lib_api.h"
#include "wlioctl.h"
#include "wlutils.h"
#include "wlioctl_utils.h"
#include "wlioctl_defs.h"
#include "bcmnvram.h"
#include "bcmendian.h"

#include "wldm_lib.h"
#include "wlutils.h"

#include "wlif_utils.h"
#include "shutils.h"
#include "wpsdefs.h"

#include "bcmwifi_rates.h"
#include "bcmwifi_rspec.h"
#include "passpoint_enc_dec.h"
#include "passpoint_nvparse.h"
#include "bcm_math.h"

static char *ratesetALL[] = {"1", "2", "5.5", "6", "9", "11", "12", "18", "24", "36", "48", "54"};
static char *ratesetOFDM[] = {"6", "9", "12", "18", "24", "36", "48", "54"};
static char *ratesetBeacon[] = {"1", "2", "5.5", "6", "11", "12", "24" };
#define MAX_NUM_RATE_SET		(sizeof(ratesetALL)/sizeof(char*))
#define MAX_NUM_OFDM_RATE_SET		(sizeof(ratesetOFDM)/sizeof(char*))
#define MAX_NUM_BEACON_RATE_SET		(sizeof(ratesetBeacon) / sizeof(char*))
struct d11_radioBasicRates_t {
	char *basicRates[MAX_NUM_RATE_SET];
	int brlen;
};

#define MAX_NUM_ANTENNA_CHAIN		4
#define ANTENNA_NUM_MASK		((1 << MAX_NUM_ANTENNA_CHAIN) -1)

#ifdef __CONFIG_DHDAP__
#include "bcmutils.h"
#include "dhdioctl.h"

extern int dhd_probe(char *name);
extern int dhd_ioctl(char *name, int cmd, void *buf, int len);
extern int dhd_bssiovar_getbuf(char *name, char *iovar, int bssidx,
	void *param, int paramlen, void *bufptr, int buflen);
#endif /* __CONFIG_DHDAP__ */

extern int wldm_get_escanlock(int radioIndex);
extern int wldm_free_escanlock(int lfd);
extern int bcm_ether_atoe(const char *p, struct ether_addr *ea);
#define HAPD_DIR "/var/run/hostapd"

typedef struct chanspec_bw_info {
	uint bw;
	char *bw_str;
	uint bw_cap;
} cspec_bw_t;

typedef struct _xbrcm {
	char *var;						/* Keyword exposed to API caller */
	union {
		void *p;
		int (*dm_func)(int cmd, int index, ...);	/* The data model function */
	};
	int cmd;						/* Supported CMD types */
} xbrcm_t;

typedef struct wldm_value_set {
	char *str_tr181;					/* TR181 string */
	char *str_nvram;					/* nvram parameter value */
	int iovar;						/* iovar parameter value */
} wldm_value_set_t;

/* Supported MFP config modes */
typedef enum {
	MFP_CONFIG_DISABLE = 0,
	MFP_CONFIG_CAPABLE,
	MFP_CONFIG_REQUIRED,
	MFP_CONFIG_TOTAL
} MFP_CONFIG_MODE;
const char * mfpModeStr[MFP_CONFIG_TOTAL] = { "Disabled", "Optional", "Required" };

static boolean remSpaceInStr(char *out_str, int out_len, const char *in_str);

extern chanspec_t wf_chspec_aton(const char *chanspec_str);
static int wl_setHEcmd(unsigned int radioIndex, unsigned short he_id, unsigned short he_len, void *heInfop);
static int wl_getHEcmd(unsigned int radioIndex, unsigned short he_id, unsigned short he_len, void *heInfop);

static int wl_setBSSColorInfo(unsigned int radioIndex, wl_he_bsscolor_t *bcp);
static int wl_getBSSColorInfo(unsigned int radioIndex, wl_he_bsscolor_t *bcp);

static int dm_mode_reqd(int cmd, int apIndex, int *pvalue, int *plen, char *pvar);
static int dm_vhtmode(int cmd, int radioIndex, int *pvalue, int *plen, char *pvar);

#define WIFIRADIO_DCSDWELLTIME_MIN	20
#define WIFIRADIO_DCSDWELLTIME_MAX	50
#define WIFIRADIO_DCSDWELLTIME_MIN_6G	110
#define WIFIRADIO_DCSDWELLTIME_MAX_6G	130
#define BOUNDS_SET(param, min, max) \
	param = (param < min) ? min : ((param > max) ? max : param)

#define CHSTRLEN			7	/* example: "0x1904," is a 7 byte string */
static int dm_csa(int cmd, int radioIndex, void *pvalue, uint *plen, char *pvar);
static int dm_dcs_dwell_time(int cmd, int radioIndex, int *pvalue, uint *plen, char *pvar);
static int dm_acs_pref_chans(int cmd, int radioIndex, char *pvalue, uint *plen, char *pvar);

static int convert_tr181_list(bool to_tr181, char *ptr181_list, int tr181_sz,
	char *pss_list, int ss_sz, int num_space_chars);
static int wl_get_chanim_stats(char *osifname, uint stats_len, uint stats_count,
	char *ioctl_buf, uint buf_size);
static int dm_xbrcm(int cmd, int index, void *pvalue, uint *plen, char *pvar,
	xbrcm_t *ptbl);
static int wl_get_assoclist(char *osifname, char *ioctl_buf, uint buf_size);
static int is11AXCapable(char *osifname);

static int wl_get_wl_band(char *osifname, int *wl_band);
static int wl_get_per_chan_info(char *osifname, uint channel, char *buf, int length);
static int wl_get_dfs_status_all(char *osifname, wl_dfs_status_all_t *dfs_status_all, int len);

#define WLDM_TRAFFIC_COUNTER_BUFLEN    2048
#ifdef CMWIFI_RDKB
#define WLDM_FILE_SYSTEM_UPTIME		"/var/systemUptime.txt"
#else
#define WLDM_FILE_SYSTEM_UPTIME		"/tmp/systemUptime.txt"
#endif /* CMWIFI_RDKB */

typedef enum {
	WPA_ENCRYPTION_MODE_TKIP = 0,
	WPA_ENCRYPTION_MODE_AES,
	WPA_ENCRYPTION_MODE_TKIP_AES,
	WPA_ENCRYPTION_MODE_TOTAL
} WPA_ENCRYPTION_MODE;

typedef enum {
	BASIC_AUTHENTICATION_MODE_NONE = 0,
	BASIC_AUTHENTICATION_MODE_EAP,
	BASIC_AUTHENTICATION_MODE_SHARED,
	BASIC_AUTHENTICATION_MODE_PSK,
	BASIC_AUTHENTICATION_MODE_TOTAL
} BASIC_AUTHENTICATION_MODE;
const char * basicAuthenticationModeStr[BASIC_AUTHENTICATION_MODE_TOTAL] = { "None", "EAPAuthentication",  "SharedAuthentication", "PSKAuthentication" };
const char * wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TOTAL] = { "TKIPEncryption", "AESEncryption", "TKIPandAESEncryption" };

#define QDBM_OFFSET		153	/* QDBM_OFFSET */
#define QDBM_TABLE_LEN		40	/* QDBM_TABLE_LEN */
#define MAX_TRANSMIT_POWER	1496	/* maximum power in mw */
#define QDBM_TABLE_LOW_BOUND	6493	/* wlu.c */
static const uint16 nqdBm_to_mW_map[QDBM_TABLE_LEN] = {
/* qdBm:        +0      +1      +2      +3      +4      +5      +6      +7      */
/* 153: */      6683,   7079,   7499,   7943,   8414,   8913,   9441,   10000,
/* 161: */      10593,  11220,  11885,  12589,  13335,  14125,  14962,  15849,
/* 169: */      16788,  17783,  18836,  19953,  21135,  22387,  23714,  25119,
/* 177: */      26607,  28184,  29854,  31623,  33497,  35481,  37584,  39811,
/* 185: */      42170,  44668,  47315,  50119,  53088,  56234,  59566,  63096
};

#define MCSTCNT_LE10_FROM_CNTBUF(cntbuf) (const wl_cnt_v_le10_mcst_t *) \
		bcm_get_data_from_xtlv_buf(((wl_cnt_info_t *)cntbuf)->data,	\
		((wl_cnt_info_t *)cntbuf)->datalen,			\
		WL_CNT_XTLV_CNTV_LE10_UCODE, NULL,			\
		BCM_XTLV_OPTION_ALIGN32)

#define MCSTCNT_GT10_FROM_CNTBUF(cntbuf) (const wl_cnt_v_le10_mcst_t *) \
		bcm_get_data_from_xtlv_buf(((wl_cnt_info_t *)cntbuf)->data,	\
		((wl_cnt_info_t *)cntbuf)->datalen,			\
		WL_CNT_XTLV_LT40_UCODE_V1, NULL,			\
		BCM_XTLV_OPTION_ALIGN32)

#define MCSTCNT_GE40_FROM_CNTBUF(cntbuf) (const wl_cnt_ge40mcst_v1_t *) \
		bcm_get_data_from_xtlv_buf(((wl_cnt_info_t *)cntbuf)->data,	\
		((wl_cnt_info_t *)cntbuf)->datalen,			\
		WL_CNT_XTLV_GE40_UCODE_V1, NULL,			\
		BCM_XTLV_OPTION_ALIGN32)

#define MCSTCNT_LT40_FROM_CNTBUF(cntbuf) (wl_cnt_lt40mcst_v1_t *)\
		bcm_get_data_from_xtlv_buf(((wl_cnt_info_t *)cntbuf)->data,     \
		((wl_cnt_info_t *)cntbuf)->datalen,                     \
		WL_CNT_XTLV_LT40_UCODE_V1, NULL,                        \
		BCM_XTLV_OPTION_ALIGN32)

typedef struct {
        const char *name;   /* Long name */
        const char *abbrev; /* Abbreviation */
} cntry_name_t;

cntry_name_t cntry_names[] = {
    {"AFGHANISTAN",     "AF"},
    {"ALBANIA",     "AL"},
    {"ALGERIA",     "DZ"},
    {"AMERICAN SAMOA",  "AS"},
    {"ANDORRA",     "AD"},
    {"ANGOLA",      "AO"},
    {"ANGUILLA",        "AI"},
    {"ANTARCTICA",      "AQ"},
    {"ANTIGUA AND BARBUDA", "AG"},
    {"ARGENTINA",       "AR"},
    {"ARMENIA",     "AM"},
    {"ARUBA",       "AW"},
    {"ASCENSION ISLAND",    "AC"},
    {"AUSTRALIA",       "AU"},
    {"AUSTRIA",     "AT"},
    {"AZERBAIJAN",      "AZ"},
    {"BAHAMAS",     "BS"},
    {"BAHRAIN",     "BH"},
    {"BANGLADESH",      "BD"},
    {"BARBADOS",        "BB"},
    {"BELARUS",     "BY"},
    {"BELGIUM",     "BE"},
    {"BELIZE",      "BZ"},
    {"BENIN",       "BJ"},
    {"BERMUDA",     "BM"},
    {"BHUTAN",      "BT"},
    {"BOLIVIA",     "BO"},
    {"BOSNIA AND HERZEGOVINA",      "BA"},
    {"BOTSWANA",        "BW"},
    {"BOUVET ISLAND",   "BV"},
    {"BRAZIL",      "BR"},
    {"BRITISH INDIAN OCEAN TERRITORY",      "IO"},
    {"BRUNEI DARUSSALAM",   "BN"},
    {"BULGARIA",        "BG"},
    {"BURKINA FASO",    "BF"},
    {"BURUNDI",     "BI"},
    {"CAMBODIA",        "KH"},
    {"CAMEROON",        "CM"},
    {"CANADA",      "CA"},
    {"CAPE VERDE",      "CV"},
    {"CAYMAN ISLANDS",  "KY"},
    {"CENTRAL AFRICAN REPUBLIC",        "CF"},
    {"CHAD",        "TD"},
    {"CHILE",       "CL"},
    {"CHINA",       "CN"},
    {"CHRISTMAS ISLAND",    "CX"},
    {"CLIPPERTON ISLAND",   "CP"},
    {"COCOS (KEELING) ISLANDS",     "CC"},
    {"COLOMBIA",        "CO"},
    {"COMOROS",     "KM"},
    {"CONGO",       "CG"},
    {"CONGO, THE DEMOCRATIC REPUBLIC OF THE",       "CD"},
    {"COOK ISLANDS",    "CK"},
    {"COSTA RICA",      "CR"},
    {"COTE D'IVOIRE",   "CI"},
    {"CROATIA",     "HR"},
    {"CUBA",        "CU"},
    {"CYPRUS",      "CY"},
    {"CZECH REPUBLIC",  "CZ"},
    {"DENMARK",     "DK"},
    {"DJIBOUTI",        "DJ"},
    {"DOMINICA",        "DM"},
    {"DOMINICAN REPUBLIC",  "DO"},
    {"ECUADOR",     "EC"},
    {"EGYPT",       "EG"},
    {"EL SALVADOR",     "SV"},
    {"EQUATORIAL GUINEA",   "GQ"},
    {"ERITREA",     "ER"},
    {"ESTONIA",     "EE"},
    {"ETHIOPIA",        "ET"},
    {"FALKLAND ISLANDS (MALVINAS)",     "FK"},
    {"FAROE ISLANDS",   "FO"},
    {"FIJI",        "FJ"},
    {"FINLAND",     "FI"},
    {"FRANCE",      "FR"},
    {"FRENCH GUIANA",   "GF"},
    {"FRENCH POLYNESIA",    "PF"},
    {"FRENCH SOUTHERN TERRITORIES",     "TF"},
    {"GABON",       "GA"},
    {"GAMBIA",      "GM"},
    {"GEORGIA",     "GE"},
    {"GERMANY",     "DE"},
    {"GHANA",       "GH"},
    {"GIBRALTAR",       "GI"},
    {"GREECE",      "GR"},
    {"GREENLAND",       "GL"},
    {"GRENADA",     "GD"},
    {"GUADELOUPE",      "GP"},
    {"GUAM",        "GU"},
    {"GUATEMALA",       "GT"},
    {"GUERNSEY",        "GG"},
    {"GUINEA",      "GN"},
    {"GUINEA-BISSAU",   "GW"},
    {"GUYANA",      "GY"},
    {"HAITI",       "HT"},
    {"HEARD ISLAND AND MCDONALD ISLANDS",       "HM"},
    {"HOLY SEE (VATICAN CITY STATE)",       "VA"},
    {"HONDURAS",        "HN"},
    {"HONG KONG",       "HK"},
    {"HUNGARY",     "HU"},
    {"ICELAND",     "IS"},
    {"INDIA",       "IN"},
    {"INDONESIA",       "ID"},
    {"IRAN, ISLAMIC REPUBLIC OF",       "IR"},
    {"IRAQ",        "IQ"},
    {"IRELAND",     "IE"},
    {"ISRAEL",      "IL"},
    {"ITALY",       "IT"},
    {"JAMAICA",     "JM"},
    {"JAPAN",       "JP"},
    {"JERSEY",      "JE"},
    {"JORDAN",      "JO"},
    {"KAZAKHSTAN",      "KZ"},
    {"KENYA",       "KE"},
    {"KIRIBATI",        "KI"},
    {"KOREA, DEMOCRATIC PEOPLE'S REPUBLIC OF",      "KP"},
    {"KOREA, REPUBLIC OF",  "KR"},
    {"KUWAIT",      "KW"},
    {"KYRGYZSTAN",      "KG"},
    {"LAO PEOPLE'S DEMOCRATIC REPUBLIC",        "LA"},
    {"LATVIA",      "LV"},
    {"LEBANON",     "LB"},
    {"LESOTHO",     "LS"},
    {"LIBERIA",     "LR"},
    {"LIBYAN ARAB JAMAHIRIYA",      "LY"},
    {"LIECHTENSTEIN",   "LI"},
    {"LITHUANIA",       "LT"},
    {"LUXEMBOURG",      "LU"},
    {"MACAO",       "MO"},
    {"MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF",      "MK"},
    {"MADAGASCAR",      "MG"},
    {"MALAWI",      "MW"},
    {"MALAYSIA",        "MY"},
    {"MALDIVES",        "MV"},
    {"MALI",        "ML"},
    {"MALTA",       "MT"},
    {"MAN, ISLE OF",    "IM"},
    {"MARSHALL ISLANDS",    "MH"},
    {"MARTINIQUE",      "MQ"},
    {"MAURITANIA",      "MR"},
    {"MAURITIUS",       "MU"},
    {"MAYOTTE",     "YT"},
    {"MEXICO",      "MX"},
    {"MICRONESIA, FEDERATED STATES OF",     "FM"},
    {"MOLDOVA, REPUBLIC OF",        "MD"},
    {"MONACO",      "MC"},
    {"MONGOLIA",        "MN"},
    {"MONTENEGRO",      "ME"},
    {"MONTSERRAT",      "MS"},
    {"MOROCCO",     "MA"},
    {"MOZAMBIQUE",      "MZ"},
    {"MYANMAR",     "MM"},
    {"NAMIBIA",     "NA"},
    {"NAURU",       "NR"},
    {"NEPAL",       "NP"},
    {"NETHERLANDS",     "NL"},
    {"NETHERLANDS ANTILLES",        "AN"},
    {"NEW CALEDONIA",   "NC"},
    {"NEW ZEALAND",     "NZ"},
    {"NICARAGUA",       "NI"},
    {"NIGER",       "NE"},
    {"NIGERIA",     "NG"},
    {"NIUE",        "NU"},
    {"NORFOLK ISLAND",      "NF"},
    {"NORTHERN MARIANA ISLANDS",        "MP"},
    {"NORWAY",      "NO"},
    {"OMAN",        "OM"},
    {"PAKISTAN",        "PK"},
    {"PALAU",       "PW"},
    {"PALESTINIAN TERRITORY, OCCUPIED",     "PS"},
    {"PANAMA",      "PA"},
    {"PAPUA NEW GUINEA",    "PG"},
    {"PARAGUAY",        "PY"},
    {"PERU",        "PE"},
    {"PHILIPPINES",     "PH"},
    {"PITCAIRN",        "PN"},
    {"POLAND",      "PL"},
    {"PORTUGAL",        "PT"},
    {"PUERTO RICO",     "PR"},
    {"QATAR",       "QA"},
    {"Q1",      "Q1"},
    {"REUNION",     "RE"},
    {"ROMANIA",     "RO"},
    {"RUSSIAN FEDERATION",  "RU"},
    {"RWANDA",      "RW"},
    {"SAINT HELENA",    "SH"},
    {"SAINT KITTS AND NEVIS",       "KN"},
    {"SAINT LUCIA",     "LC"},
    {"SAINT PIERRE AND MIQUELON",       "PM"},
    {"SAINT VINCENT AND THE GRENADINES",        "VC"},
    {"SAMOA",       "WS"},
    {"SAN MARINO",      "SM"},
    {"SAO TOME AND PRINCIPE",       "ST"},
    {"SAUDI ARABIA",    "SA"},
    {"SENEGAL",     "SN"},
    {"SERBIA",      "RS"},
    {"SEYCHELLES",      "SC"},
    {"SIERRA LEONE",    "SL"},
    {"SINGAPORE",       "SG"},
    {"SLOVAKIA",        "SK"},
    {"SLOVENIA",        "SI"},
    {"SOLOMON ISLANDS", "SB"},
    {"SOMALIA",     "SO"},
    {"SOUTH AFRICA",    "ZA"},
    {"SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS",        "GS"},
    {"SPAIN",       "ES"},
    {"SRI LANKA",       "LK"},
    {"SUDAN",       "SD"},
    {"SURINAME",        "SR"},
    {"SVALBARD AND JAN MAYEN",      "SJ"},
    {"SWAZILAND",       "SZ"},
    {"SWEDEN",      "SE"},
    {"SWITZERLAND",     "CH"},
    {"SYRIAN ARAB REPUBLIC",        "SY"},
    {"TAIWAN, PROVINCE OF CHINA",       "TW"},
    {"TAJIKISTAN",      "TJ"},
    {"TANZANIA, UNITED REPUBLIC OF",        "TZ"},
    {"THAILAND",        "TH"},
    {"TIMOR-LESTE (EAST TIMOR)",        "TL"},
    {"TOGO",        "TG"},
    {"TOKELAU",     "TK"},
    {"TONGA",       "TO"},
    {"TRINIDAD AND TOBAGO", "TT"},
    {"TRISTAN DA CUNHA",    "TA"},
    {"TUNISIA",     "TN"},
    {"TURKEY",      "TR"},
    {"TURKMENISTAN",    "TM"},
    {"TURKS AND CAICOS ISLANDS",        "TC"},
    {"TUVALU",      "TV"},
    {"UGANDA",      "UG"},
    {"UKRAINE",     "UA"},
    {"UNITED ARAB EMIRATES",        "AE"},
    {"UNITED KINGDOM",  "GB"},
    {"UNITED STATES",   "US"},
    {"UNITED STATES MINOR OUTLYING ISLANDS",        "UM"},
    {"URUGUAY",     "UY"},
    {"UZBEKISTAN",      "UZ"},
    {"VANUATU",     "VU"},
    {"VENEZUELA",       "VE"},
    {"VIET NAM",        "VN"},
    {"VIRGIN ISLANDS, BRITISH",     "VG"},
    {"VIRGIN ISLANDS, U.S.",        "VI"},
    {"WALLIS AND FUTUNA",   "WF"},
    {"WESTERN SAHARA",  "EH"},
    {"YEMEN",       "YE"},
    {"YUGOSLAVIA",      "YU"},
    {"ZAMBIA",      "ZM"},
    {"ZIMBABWE",        "ZW"},
    {"RADAR CHANNELS",  "RDR"},
    {"ALL CHANNELS",    "ALL"},
    {NULL,          NULL}
};

static uint16
wl_qdbm_to_mw(int8 qdbm)
{
	uint factor = 1;
	int idx = qdbm - QDBM_OFFSET;

	if (idx >= QDBM_TABLE_LEN) {
		/* clamp to max uint16 mW value */
		return 0xFFFF;
	}

	/* scale the qdBm index up to the range of the table 0-40
	 * where an offset of 40 qdBm equals a factor of 10 mW.
	 */
	while (idx < 0) {
		idx += 40;
		factor *= 10;
	}

	/* return the mW value scaled down to the correct factor of 10,
	 * adding in factor/2 to get proper rounding.
	 */
	return ((nqdBm_to_mW_map[idx] + factor/2) / factor);
}

static int8
wl_mw_to_qdbm(uint16 mw)
{
        uint8 qdbm;
        int offset;
        uint mw_uint = mw;
        uint boundary;

        /* handle boundary case */
        if (mw_uint <= 1)
                return WL_RATE_DISABLED;

        offset = QDBM_OFFSET;

        /* move mw into the range of the table */
        while (mw_uint < QDBM_TABLE_LOW_BOUND) {
                mw_uint *= 10;
                offset -= 40;
        }

        for (qdbm = 0; qdbm < (QDBM_TABLE_LEN - 1); qdbm++) {
                boundary = nqdBm_to_mW_map[qdbm] +
                        (nqdBm_to_mW_map[qdbm+1] - nqdBm_to_mW_map[qdbm])/2;
                if (mw_uint < boundary) break;
        }

        qdbm += (int8)offset;

        return (qdbm);
}

static int
syscmd(char *cmd, char *retBuf, int retBufSize) {
	FILE *f;
	char *ptr = retBuf;
	int bufSize = 0, readbytes = 0;

	if (!retBuf || !(retBufSize)) {
		WIFI_ERR("%s invalid retBuf(%s) or retBufSize (%d)\n",
			__FUNCTION__, (!retBuf) ? "null" : "not null", retBufSize);
		return -1;
	}
	if ((f = popen(cmd, "r")) == NULL) {
		WIFI_ERR("popen %s error\n", cmd);
		return -1;
	}

	while (!feof(f) && ((retBufSize - bufSize) > 1)) {
		if (fgets(ptr, (retBufSize - bufSize), f) == NULL) {
			WIFI_ERR("%s: fgets failed\n", __FUNCTION__);
		}
		readbytes = strlen(ptr);
		if (readbytes == 0)
			break;
		bufSize += readbytes;
		ptr += readbytes;
	}

	pclose(f);

	if (bufSize > 0) {
		retBuf[bufSize - 1] = '\0';
	} else {
		retBuf[0] = '\0';
	}
	return 0;
}

/*
 * Definition conflicts with the one in libshared.so.
 * To workaround compile issue, rename it.
 * TODO: To use the one in libshared.so
 */
int
_wl_ether_atoe(const char *a, struct ether_addr *n)
{
	char *c = NULL;
	int i = 0;

	memset(n, 0, ETHER_ADDR_LEN);
	for (;;) {
		n->octet[i++] = (uint8)strtoul(a, &c, 16);
		if (!*c++ || i == ETHER_ADDR_LEN)
			break;
		a = c;
	}
	return (i == ETHER_ADDR_LEN);
}

char *
wl_ether_etoa(const struct ether_addr *n)
{
	static char etoa_buf[ETHER_ADDR_LEN * 3];
	char *c = etoa_buf;
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (i)
			*c++ = ':';
		c += sprintf(c, "%02x", n->octet[i] & 0xff);
	}
	return etoa_buf;
}

#define WL0_IFNAME			"wl0"
#define WL1_IFNAME			"wl1"
#define WL2_IFNAME			"wl2"

static char *_wl_ifname[MAX_WLAN_ADAPTER] = {
	WL0_IFNAME,
#if (MAX_WLAN_ADAPTER >= 2)
	WL1_IFNAME,
#endif /* MAX_WLAN_ADAPTER >= 2 */
#if (MAX_WLAN_ADAPTER >= 3)
	WL2_IFNAME,
#endif /* MAX_WLAN_ADAPTER >= 3 */
};

#define NEIGHBORDIAG_MODE_MANAGED					"Managed"
#define NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_U			"u"
#define NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_L			"l"
#define NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_80			"80"
#define NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_160			"160"
#define NEIGHBORDIAG_RSN						"RSN"
#define NEIGHBORDIAG_WPA						"WPA"
#define NEIGHBORDIAG_WPA2						"WPA2"
#define NEIGHBORDIAG_WPA3						"WPA3"
#define NEIGHBORDIAG_UNICAST_CIPHERS					"unicast ciphers"
#define NEIGHBORDIAG_TKIP						"TKIP"
#define NEIGHBORDIAG_AES						"AES"
#define NEIGHBORDIAG_AKM_SUITES						"AKM Suites"
#define NEIGHBORDIAG_WPA_PSK						"WPA-PSK"
#define NEIGHBORDIAG_WPA2_PSK						"WPA2-PSK"
#define NEIGHBORDIAG_CAPABILITY						"Capability"
#define NEIGHBORDIAG_WEP						"WEP"
#define NEIGHBORDIAG_HT_CAPABLE						"HT Capable"
#define NEIGHBORDIAG_VHT_CAPABLE					"VHT Capable"
#define NEIGHBORDIAG_HE_CAPABLE						"HE Capable"

#define NEIGHBORDIAG_DTIMPERIOD_DEFAULTS				1
#define NEIGHBORDIAG_BEACONPERIOD_DEFAULTS				100
#define WIFI_LOWEST_5G_CHANNEL_NUMBER					36
#define WIFI_HIGHEST_2_4_G_CHANNEL_NUMBER				14

typedef enum _SCANPARAMS_NODEIDX_ {
	SCANPARAMS_SCANMODE = 0,
	SCANPARAMS_DWELLTIME,
	SCANPARAMS_NUMBEROFCHANNELS,
	SCANPARAMS_CHANNELLIST,
	SCANPARAMS_MAX
} SCANPARAMS_NODEIDX;

typedef struct _wldm_nodeIdx_wlKeyword_mapping {
	uint		node_idx;
	char		*wl_keyword;
} wldm_nodeIdx_wlKeyword_mapping_t;

typedef enum _NEIGHBORDIAG_SUBNODE_ {
	NEIGHBORDIAG_SSID,
	NEIGHBORDIAG_BSSID,
	NEIGHBORDIAG_MODE,
	NEIGHBORDIAG_CHANNEL,
	NEIGHBORDIAG_SIGNALSTRENGTH,
	NEIGHBORDIAG_SECURITYMODEENABLED,
	NEIGHBORDIAG_ENCRYPTIONMODE,
	NEIGHBORDIAG_NOISE,
	NEIGHBORDIAG_SUPPORTEDRATES,
	NEIGHBORDIAG_QBSSUTILIZATION,
	NEIGHBORDIAG_MAX_SUBNODES
} NEIGHBORDIAG_SUBNODE;

typedef enum _ESCANFAILS_SUBNODE_ {
	ESCANFAILS_SCANREJECTED = 0,
	ESCANFAILS_MISCERROR,
	ESCANFAILS_STATUS,
	ESCANFAILS_MAX_SUBNODES
} ESCANFAILS_SUBNODE;

typedef enum _NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_IDX {
	NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_20MHZ,
	NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_40MHZ,
	NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_80MHZ,
	NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_160MHZ,
	NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_AUTO,
	NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_MAX
} NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_IDX;

typedef enum NEIGHBORDIAG_SECURITYMODEENABLED_IDX {
	NEIGHBORDIAG_SECURITYMODEENABLED_NONE,
	NEIGHBORDIAG_SECURITYMODEENABLED_WEP,
	NEIGHBORDIAG_SECURITYMODEENABLED_WPA,
	NEIGHBORDIAG_SECURITYMODEENABLED_WPA2,
	NEIGHBORDIAG_SECURITYMODEENABLED_WPA_WPA2,
	NEIGHBORDIAG_SECURITYMODEENABLED_WPA_ENTERPRISE,
	NEIGHBORDIAG_SECURITYMODEENABLED_WPA2_ENTERPRISE,
	NEIGHBORDIAG_SECURITYMODEENABLED_WPA_WPA2_ENTERPRISE,
	NEIGHBORDIAG_SECURITYMODEENABLED_WPA3_PERSONAL_TRANSITION,
	NEIGHBORDIAG_SECURITYMODEENABLED_WPA3_PERSONAL,
} NEIGHBORDIAG_SECURITYMODEENABLED_IDX;

typedef enum NEIGHBORDIAG_ENCRYPTIONMODE_IDX {
	NEIGHBORDIAG_ENCRYPTIONMODE_TKIP,
	NEIGHBORDIAG_ENCRYPTIONMODE_AES,
	NEIGHBORDIAG_ENCRYPTIONMODE_TKIPandAES
} NEIGHBORDIAG_ENCRYPTIONMODE_IDX;

static const wldm_nodeIdx_wlKeyword_mapping_t wifi_scanParams_mapTable[] = {
	{ SCANPARAMS_SCANMODE,					"Scan Mode:" },
	{ SCANPARAMS_DWELLTIME,					"Dwell Time:" },
	{ SCANPARAMS_NUMBEROFCHANNELS,				"Number Of Channels:" },
	{ SCANPARAMS_CHANNELLIST,				"Channel List:[ " }
};

static const wldm_nodeIdx_wlKeyword_mapping_t wifi_neighborDiagsRes_mapTable[] = {
	{ NEIGHBORDIAG_SSID,					"SSID: " },
	{ NEIGHBORDIAG_BSSID,					"BSSID: " },
	{ NEIGHBORDIAG_MODE,					"Mode: " },
	{ NEIGHBORDIAG_CHANNEL,					"Channel: " },
	{ NEIGHBORDIAG_SIGNALSTRENGTH,				"RSSI: " },
	{ NEIGHBORDIAG_SECURITYMODEENABLED,			"AKM Suites" },
	{ NEIGHBORDIAG_ENCRYPTIONMODE,				"unicast ciphers" },
	{ NEIGHBORDIAG_NOISE,					"noise: " },
	{ NEIGHBORDIAG_SUPPORTEDRATES,				"Supported Rates: " },
	{ NEIGHBORDIAG_QBSSUTILIZATION,				"QBSS Channel Utilization: " }
};

static const wldm_nodeIdx_wlKeyword_mapping_t wifi_escanErrors_mapTable[] = {
	{ ESCANFAILS_SCANREJECTED,				"Scan Rejected" },
	{ ESCANFAILS_MISCERROR,					", misc. error/abort" },
	{ ESCANFAILS_STATUS,					", status:" }
};

#define NEIGHBOR_SECURITY_MODE_MAX				10
static char *neighborSecurityMode[NEIGHBOR_SECURITY_MODE_MAX] = { "None", "WEP", "WPA", "WPA2",
	"WPA-WPA2", "WPA-Enterprise", "WPA2-Enterprise", "WPA-WPA2-Enterprise",
	"WPA3-Personal-Transition", "WPA3-Personal" };

#define NEIGHBOR_ENC_MODE_MAX					3
static char *neighborEncryptionMode[NEIGHBOR_ENC_MODE_MAX] = { "TKIP", "AES", "TKIPandAES" };

static char *WLDM_OPERATINGBANDWIDTH[NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_MAX] =
	{ "20", "40", "80", "160", "Auto" };

#define ESCANRESULTS_CMD					"wl -i wl%d escanresults "
#define SCAN_TYPE_OPTION					"-t %s "
#define SCAN_TYPE_OFFCHAN					"offchan "
#define SCAN_TYPE_PASSIVE					"passive "
#define DWELL_TIME_ACTIVE_OPTION				"-a %d "
#define DWELL_TIME_PASSIVE_OPTION				"-p %d "
#define CHANNEL_LIST_OPTION					"-c %s "
#define CHANNEL_LIST_OPTION_UI32				"-c %d "

/* Driver access routines. */
int
wlif_idx(char *osifname)
{
	int index = -1;

	if (wl_iovar_getint(osifname, "bsscfg_idx", (int *)&index) < 0)
		return -1;

	return index;
}

static int
is_ap(char *osifname)
{
	int ap_mode = 0;

	if (wl_ioctl(osifname, WLC_GET_AP, &ap_mode, sizeof(ap_mode)) < 0)
		return 0;

	return (ap_mode != 0);
}

static int
radios_get(void)
{
	int i, radios = 0;
	char *nvifname, osifname[IFNAMSIZ];

	for (i = 0; i < MAX_WLAN_ADAPTER; i++) {
		/* Find out how many radios exist in the system */
		nvifname = _wl_ifname[i];
		if (!nvifname)
			break;
		nvifname_to_osifname(nvifname, osifname, sizeof(osifname));
		if (wl_probe(osifname) != 0)
			continue;
		radios++;
	}
	return radios;
}

#ifdef BCA_CPEROUTER_RDK
#define VALUE_NOT_ASSIGNED (-1)
static int wldm_mbss_nvram_set(char *name, char *osifname, boolean enabled)
{
	int idx = 0, ssid_idx = 0;
	if (get_ifname_unit(name, &idx, &ssid_idx) == 0 && ssid_idx > 0) {
		/* if name is mbss name in wlx.y format,keep going. */
		char nvname[NVRAM_NAME_SIZE], buf[BUF_SIZE] = {0}, *nvram_value;
		int default_bridge = VALUE_NOT_ASSIGNED, index;
		/* set wlx.y_ifname */
		snprintf(nvname, sizeof(nvname), "%s_ifname", name);
		if (enabled)
			NVRAM_SET(nvname, osifname);
		else
			nvram_unset(nvname);

		/* set primary wlx_vifs  */
		snprintf(nvname, sizeof(nvname), "wl%d_vifs", idx);
		nvram_value=nvram_get(nvname);
		if (find_in_list(nvram_value, name)) {
			/* if disabled and removed */
			if (!enabled && !remove_from_list(name, nvram_value, strlen(nvram_value)))  {
				NVRAM_SET(nvname, nvram_value);
			}
		} else if (enabled) {
			if (!nvram_value || nvram_value[0] == '\0')
				snprintf(buf, sizeof(buf), "%s", name);
			else
				snprintf(buf, sizeof(buf), "%s %s", name, nvram_value);
			NVRAM_SET(nvname, buf);
		}

		/* set lanx_ifnames or lan_ifnames, searching if wlx.y is already in any lanx_ifnames or not and get
		 * primary radio's default bridge number
		 */
		for (index = 0; index < WLIFU_MAX_NO_BRIDGE; index++) {
			if (index == 0)
				snprintf(nvname, NVRAM_NAME_SIZE, "lan_ifnames");
			else
				snprintf(nvname, NVRAM_NAME_SIZE, "lan%d_ifnames", index);
			if ((nvram_value = nvram_get(nvname)) && nvram_value[0] != '\0') {
				if (default_bridge == VALUE_NOT_ASSIGNED) {
					snprintf(buf, NVRAM_NAME_SIZE, "wl%d", idx);
					if (find_in_list(nvram_value, buf)) default_bridge = index;
				}
				if (find_in_list(nvram_value, name)) break;
			}
		}

		if (index == WLIFU_MAX_NO_BRIDGE) {
			if (enabled) {
				/* interface is not in any bridge, put it into default as primary wifi interface */
				if (default_bridge == 0 || default_bridge == VALUE_NOT_ASSIGNED)
					snprintf(nvname, NVRAM_NAME_SIZE, "lan_ifnames");
				else
					snprintf(nvname, NVRAM_NAME_SIZE, "lan%d_ifnames", default_bridge);
				if ((nvram_value = nvram_get(nvname)))
					snprintf(buf, sizeof(buf), "%s %s", nvram_value, name);
				else
					snprintf(buf, sizeof(buf), "%s", name);
				NVRAM_SET(nvname, buf);
			}
		} else if (!enabled) {
			/* when it include in lanx_ifnames and disabld, remove from the nvram  */
			if (!remove_from_list(name, nvram_value, strlen(nvram_value))) {
				NVRAM_SET(nvname, nvram_value);
			}
		}
	}
	return 0;
}
#endif

#define AP_SSID			1
#define EP_SSID			2
static int
SSIDs_get(int mode)
{
	int i, val, SSIDs, ap;
	char *osifname;

	for (i = 0, SSIDs = 0; i < wldm_get_max_aps(); i++) {
		osifname = wldm_get_osifname(i);
		if (wl_ioctl(osifname, WLC_GET_MAGIC, &val, sizeof(val)) < 0)
			continue;
		ap = is_ap(osifname);
		if (ap && (mode & AP_SSID))
			SSIDs++;
		if (!ap && (mode & EP_SSID))
			SSIDs++;
	}
	return SSIDs;
}

/**************
*  Device.WiFi.
**************/
int
wldm_RadioNumberOfEntries(int cmd, int index,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RadioNumberOfEntries";

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_GET_NVRAM |
		CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		*pvalue = radios_get();
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_SSIDNumberOfEntries(int cmd, int index,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SSIDNumberOfEntries";

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_GET_NVRAM |
		CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		*pvalue = SSIDs_get(AP_SSID | EP_SSID);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_AccessPointNumberOfEntries(int cmd, int index,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AccessPointNumberOfEntries";

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_GET_NVRAM |
		CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		*pvalue = SSIDs_get(AP_SSID);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_EndPointNumberOfEntries(int cmd, int index,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "EndPointNumberOfEntries";

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_GET_NVRAM |
		CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		*pvalue = SSIDs_get(EP_SSID);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

/************************
*  Device.WiFi.Radio.{i}.
************************/
int
wldm_Radio_Enable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Enable";
	char *nvifname, *osifname, buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		int val = 0;
		*pvalue = 1; /* enabled by default */
		if (wl_ioctl(osifname, WLC_GET_RADIO, &val, sizeof(val)) < 0) {
			WIFI_ERR("%s: Failed to get radio setting\n", __FUNCTION__);
			return -1;
		}
		if (val & (WL_RADIO_SW_DISABLE | WL_RADIO_HW_DISABLE))
			*pvalue = 0;

		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_Enable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Radio.Enable = *pvalue ? 1 : 0;
		pObj->apply_map |= Radio_Enable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_radio", nvifname);
		snprintf(buf, sizeof(buf), "%d", *pvalue ? 1 : 0);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_GET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE], *nvVal;

		snprintf(nvram_name, sizeof(nvram_name), "%s_radio", nvifname);
		if ((nvVal = wlcsm_nvram_get(nvram_name))) {
			*pvalue = atoi(nvVal) ? 1 : 0;
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n",
				__FUNCTION__, nvram_name);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		if (*pvalue) {
			int val = 0;
			/* check "radio" run-time state */
			if (wl_ioctl(osifname, WLC_GET_RADIO, &val, sizeof(val)) < 0) {
				WIFI_ERR("%s: Failed to get radio setting\n", __FUNCTION__);
				return -1;
			}
			WIFI_DBG("%s: WLC_GET_RADIO 0x%x on %s\n",
				__FUNCTION__, val, osifname);

			/* in case not enabled, need enable "radio" first */
			if (val & (WL_RADIO_SW_DISABLE | WL_RADIO_HW_DISABLE)) {
				val = (WL_RADIO_SW_DISABLE | WL_RADIO_HW_DISABLE) << 16;
				WIFI_DBG("%s: WLC_SET_RADIO 0x%x on %s\n",
					__FUNCTION__, val, osifname);

				if (wl_ioctl(osifname, WLC_SET_RADIO, &val, sizeof(val)) < 0) {
					WIFI_ERR("%s: Failed to set radio setting\n", __FUNCTION__);
					return -1;
				}
			}
		}

		if (wl_ioctl(osifname, *pvalue ? WLC_UP : WLC_DOWN, NULL, 0) < 0) {
			WIFI_ERR("%s: Failed to set wl %s\n",
				__FUNCTION__, *pvalue ? "up" : "down");
			return -1;
		}
	}

	return 0;
}

int
wldm_Radio_Alias(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Alias";
	char *nvifname, *nvram_value, nvram_name[NVRAM_NAME_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_Radio.Alias", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value == NULL)
			nvram_value = "";
		if (*plen < strlen(nvram_value))
			return -1;
		strcpy(pvalue, nvram_value);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n",
				(cmd & CMD_GET) ? parameter : nvram_name, pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_Alias_MASK);
		if (pObj == NULL)
			return -1;

		if (*plen > TR181_STR_ALIAS) {
			pObj->reject_map |= Radio_Alias_MASK;
		} else {
			pObj->Radio.Alias = malloc(*plen + 1);
			memcpy(pObj->Radio.Alias, pvalue, *plen);
			pObj->Radio.Alias[*plen] = '\0';
			pObj->apply_map |= Radio_Alias_MASK;
		}
		wldm_rel_Object(pObj, (pObj->apply_map & Radio_Alias_MASK) ? TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_Radio.Alias", nvifname);
		if (wlcsm_nvram_set(nvram_name, pvalue) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_Radio_FragmentationThreshold(int cmd, int radioIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "FragmentationThreshold";
	char *nvifname, *osifname, buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "fragthresh", (int *)pvalue) < 0) {
			WIFI_ERR("%s: wl_iovar_getint() failed!\n", __FUNCTION__);
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex,
			Radio_FragmentationThreshold_MASK);
		if (pObj == NULL)
			return -1;

		if (*pvalue > 65535) {
			pObj->reject_map |= Radio_FragmentationThreshold_MASK;
		} else {
			pObj->Radio.FragmentationThreshold = *pvalue;
			pObj->apply_map |= Radio_FragmentationThreshold_MASK;
		}
		wldm_rel_Object(pObj, (pObj->apply_map & Radio_FragmentationThreshold_MASK) ?
			TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_frag", nvifname);
		snprintf(buf, sizeof(buf), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		if (wl_iovar_setint(osifname, "fragthresh", (int) *pvalue)) {
			WIFI_DBG("%s: wl_iovar_setint fragthresh failed !\n", __FUNCTION__);
		}
	}

	return 0;
}

int
wldm_Radio_RTSThreshold(int cmd, int radioIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RTSThreshold";
	char *nvifname, *osifname, buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "rtsthresh", (int *)pvalue) < 0) {
			WIFI_ERR("%s: wl_iovar_getint() failed!\n", __FUNCTION__);
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_RTSThreshold_MASK);
		if (pObj == NULL)
			return -1;

		if (*pvalue > WL_MAX_RTS_THRESHOLD) {
			pObj->reject_map |= Radio_RTSThreshold_MASK;
		} else {
			pObj->Radio.RTSThreshold = *pvalue;
			pObj->apply_map |= Radio_RTSThreshold_MASK;
		}
		wldm_rel_Object(pObj, (pObj->apply_map & Radio_RTSThreshold_MASK) ? TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_rts", nvifname);
		snprintf(buf, sizeof(buf), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		if (*pvalue > WL_MAX_RTS_THRESHOLD) {
			WIFI_DBG("%s: %d is invalid rtsthresh value!\n",
					__FUNCTION__, (int)*pvalue);
			return -1;
		}

		if (wl_iovar_setint(osifname, "rtsthresh", (int) *pvalue) < 0) {
			WIFI_DBG("%s: wl_iovar_setint rtsthresh to %d failed !\n",
					__FUNCTION__, (int)*pvalue);
			return -2;
		}
	}

	return 0;
}

static int
wl_validate_channel(int radioIndex, unsignedInt channel)
{
	int count, band = 1;
	wl_uint32_list_t *list = NULL;
	uint32 buf[WL_NUMCHANNELS + 1] = {0};
	char *osifname;

	osifname = wldm_get_radio_osifname(radioIndex);
	list = (wl_uint32_list_t *) buf;
	list ->count = htod32(WL_NUMCHANNELS);
	if (wl_ioctl(osifname, WLC_GET_VALID_CHANNELS, buf, sizeof(buf)) < 0) {
		WIFI_ERR("%s: IOVAR to get valid channels failed\n", __FUNCTION__);
		return -1;
	}
	list->count = dtoh32(list->count);
	if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
		WIFI_ERR("%s: IOVAR to get band info failed\n", __FUNCTION__);
		return -1;
	}
	band = dtoh32(band);
	if ((band == WLC_BAND_AUTO) || (band == WLC_BAND_2G)
			|| (band == WLC_BAND_5G) || (band == WLC_BAND_6G)) {
		for (count = 0; count < list->count; count++)	{
			if (list->element[count] == channel) {
				WIFI_DBG("%s: Channel=%d is valid, band=%s\n",
					 __FUNCTION__, channel,
					(band == WLC_BAND_AUTO) ? "Auto" :
					((band == WLC_BAND_2G) ? "2G" :
					((band == WLC_BAND_5G) ? "5G" :
					((band == WLC_BAND_6G) ? "6G" : "N/A"))));
				return 0;
			}
		}
	} else {
		WIFI_ERR("%s: Invalid Band=%d, OR  channel=%d\n", __FUNCTION__, band, channel);
	}
	return -1;
}

static int
wl_gen_40MHZ_chanspec(int radioIndex, unsignedInt channel, char *chanspec_str, int len)
{
	int band = 1, i;
	char *osifname, *prefix;
	unsignedInt chans_40MHZ_5G[] = {36, 44, 52, 60, 100, 108, 116, 124, 132, 140, 149, 157, 161,
					165, 169, 173, 177};
	unsignedInt chans_40MHZ_6G[] = {1, 2, 5, 9, 13, 17, 21, 25,
					29, 33, 37, 41, 45, 49, 53, 57,
					61, 65, 69, 73, 77, 81, 85, 89,
					93, 97, 101, 105, 109, 113, 117, 121,
					125, 129, 133, 137, 141, 145, 149, 153,
					157, 161, 165, 169, 173, 177, 181, 185,
					189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233};

	osifname = wldm_get_radio_osifname(radioIndex);
	if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
		WIFI_ERR("%s: IOVAR Failed to get band info\n", __FUNCTION__);
		return -1;
	}
	band = dtoh32(band);
	if ((band == WLC_BAND_AUTO) || (band == WLC_BAND_2G)) {
		if ((channel >= 1) && (channel <= 7)) {
			snprintf(chanspec_str, len, "%d%s", channel, "l");
		} else if ((channel >= 8) && (channel <= 13)) {
			snprintf(chanspec_str, len, "%d%s", channel, "u");
		}
	} else if (band == WLC_BAND_5G) {
		for (i = 0; i < ARRAY_SIZE(chans_40MHZ_5G); i++) {
			if (chans_40MHZ_5G[i] == channel) {
				snprintf(chanspec_str, len, "%d%s", channel, "l");
				break;
			} else if ((chans_40MHZ_5G[i] + 4) == channel) {
				snprintf(chanspec_str, len, "%d%s", channel, "u");
				break;
			}
		}
	} else if (band == WLC_BAND_6G) {
		prefix = "6g";
		for (i = 0; i < ARRAY_SIZE(chans_40MHZ_6G); i++) {
			if (chans_40MHZ_6G[i] == channel) {
				snprintf(chanspec_str, len, "%s%d%s", prefix, channel, "l");
				break;
			} else if ((chans_40MHZ_6G[i] + 4) == channel) {
				snprintf(chanspec_str, len, "%s%d%s", prefix, channel, "u");
				break;
			}
		}
	} else {
		WIFI_ERR("%s: Invalid Band=%d\n", __FUNCTION__, band);
		return -1;
	}
	return 0;
}

static int
wl_gen_chanspec_str(int radioIndex, unsignedInt channel, char *chanspec_str, int len,
	int bw, int extChan)
{
	char bw_cfg[NVRAM_NAME_SIZE], *prefix;
	char *key, *osifname = wldm_get_radio_osifname(radioIndex);
	int bw_cap = 0, band;

	if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
		WIFI_ERR("%s: IOVAR Failed to get band info\n", __FUNCTION__);
		return -1;
	}
	band = dtoh32(band);
	prefix = (band == WLC_BAND_6G) ? "6g" : "";

	WIFI_DBG("%s: channel=%d, bw=%d, extChan=%d\n", __FUNCTION__,
		channel, bw, extChan);
	switch (bw) {
		case 160:
		case 80:
			if (band == WLC_BAND_2G) {
				WIFI_ERR("%s: %dMHz bandwidth not supported on 2G band\n", __FUNCTION__, bw);
				return -1;
			}
			snprintf(chanspec_str, len, "%s%d/%d", prefix, channel, bw);
			break;
		case 40:
			if (extChan == 1) {
				snprintf(chanspec_str, len, "%s%d%s", prefix, channel, "u");
			} else if (extChan == -1) {
				snprintf(chanspec_str, len, "%s%d%s", prefix, channel, "l");
			} else if (extChan == 0) {
				wl_gen_40MHZ_chanspec(radioIndex, channel, chanspec_str, len);
			}
			break;
		case 20:
			snprintf(chanspec_str, len, "%s%d", prefix, channel);
			break;
		default:
			snprintf(bw_cfg, sizeof(bw_cfg), "wl%d_bw_cap", radioIndex);
			key = wlcsm_nvram_get(bw_cfg);
			bw_cap = atoi(key);

			if (0xf == bw_cap) {
				snprintf(chanspec_str, len, "%s%d%s", prefix, channel, "/160");
			} else if (7 == bw_cap) {
				snprintf(chanspec_str, len, "%s%d%s", prefix, channel, "/80");
			} else if (3 == bw_cap) {
				wl_gen_40MHZ_chanspec(radioIndex, channel, chanspec_str, len);
			} else {
				snprintf(chanspec_str, len, "%s%d", prefix, channel);
			}
			break;
	}
	return 0;
}

static int
wl_validate_chanspec(int radioIndex, unsignedInt chanspec)
{
	chanspec_t c = 0;
	int rc, i;
	wl_uint32_list_t *list;
	char buf[WLC_IOCTL_MAXLEN];
	char *osifname;

	osifname = wldm_get_radio_osifname(radioIndex);
	memset(buf, 0, WLC_IOCTL_MAXLEN);
	rc = wl_iovar_getbuf(osifname, "chanspecs", &c, sizeof(c), buf, WLC_IOCTL_MAXLEN);
	if (rc < 0) {
		WIFI_ERR("%s: Failed to get IOCTL for chanspecs\n", __FUNCTION__);
		return -1;
	}

	/* After getting the supported chanspecs list, compare with chanspec to validate */
	list = (wl_uint32_list_t *)buf;
	WIFI_DBG("%s: Number of chanspecs returned = %d\n", __FUNCTION__, dtoh32(list->count));
	for (i = 0; i < dtoh32(list->count); i++) {
		c = dtoh32(list->element[i]);
		if (c == chanspec) {
			WIFI_DBG("%s: Found Valid Chanspec=0x%04x\n", __FUNCTION__, chanspec);
			return 0;
		}
	}
	return -1;
}

static
cspec_bw_t cspec_bw[] = {
	{ WL_CHANSPEC_BW_20,	"20MHz",	WLC_BW_CAP_20MHZ},
	{ WL_CHANSPEC_BW_40,	"40MHz",	WLC_BW_CAP_40MHZ},
	{ WL_CHANSPEC_BW_80,	"80MHz",	WLC_BW_CAP_80MHZ},
	{ WL_CHANSPEC_BW_8080,	"80+80MHz",	WLC_BW_CAP_80MHZ},
	{ WL_CHANSPEC_BW_160,	"160MHz",	WLC_BW_CAP_160MHZ},
};

static int
chanspec2bandwidth(chanspec_t chanspec, char *bandwidth, int len)
{
	int i;
	unsignedInt current_bw = (chanspec & WL_CHANSPEC_BW_MASK);

	for (i = 0; i < ARRAY_SIZE(cspec_bw); i++) {
		if (cspec_bw[i].bw == current_bw) {
			if (len > (strlen(cspec_bw[i].bw_str) + 1)) {
				snprintf(bandwidth, len, "%s", cspec_bw[i].bw_str);
				break;
			}
		}
	}

	if (i >= (sizeof(cspec_bw) / sizeof(cspec_bw[0]))) {
		snprintf(bandwidth, len, "N/A");
		WIFI_ERR("%s: Invalid current_bw=%d, chanspec=0x%04x\n",
			__FUNCTION__, current_bw, chanspec);
		return -1;
	}

	return 0;
}

static int
wl_parse_chanspec(chanspec_t chanspec, wldm_channel_t* chan_ptr)
{
	bzero(chan_ptr, sizeof(*chan_ptr));
	GET_ALL_EXT(chanspec, (uint8 *) chan_ptr);
	return 0;
}

static int
wl_get_chanspecs_list(int radioIndex, uint chanspecs_len,
	chanspec_t *chanspecs, uint8 *control_channels, int *chanspecs_count)
{
	int rc, i;
	wl_uint32_list_t *list;
	char *osifname, buf[WLC_IOCTL_MAXLEN];
	chanspec_t c = 0;
	wldm_channel_t chan;

	osifname = wldm_get_radio_osifname(radioIndex);
	memset(buf, 0, sizeof(buf));
	rc = wl_iovar_getbuf(osifname, "chanspecs", &c, sizeof(c), buf, WLC_IOCTL_MAXLEN);
	if (rc < 0) {
		WIFI_ERR("%s: Failed to get IOCTL for chanspecs\n", __FUNCTION__);
		return -1;
	}

	/* After getting the supported chanspecs list, compare with chanspec to validate */
	list = (wl_uint32_list_t *)buf;
	WIFI_DBG("%s: Number of chanspecs returned = %d\n", __FUNCTION__, dtoh32(list->count));
	*chanspecs_count = dtoh32(list->count);

	if (dtoh32(list->count) > WLDM_MAX_CH_LIST_LEN) {
		WIFI_ERR("%s: chanspecs count > %d\n", __FUNCTION__, WLDM_MAX_CH_LIST_LEN);
		return -1;
	}
	for (i = 0; i < dtoh32(list->count); i++) {
		chanspecs[i] = dtoh32(list->element[i]);
		memset(&chan, 0, sizeof(wldm_channel_t));
		wl_parse_chanspec(chanspecs[i], &chan);
		control_channels[i] = chan.control;
	}
	return 0;
}

/* Validate each input channels or chanspecs, then convert them to list of excluded chanspecs
 * chanspecs_len is initially sizeof(out_chanspecs) then updated to strlen(out_chanspecs)
 */
static int
wl_get_excl_chanspecs(int radioIndex, char *inp_channels, int channels_len,
	char *out_chanspecs, uint *chanspecs_len)
{
	int i, chanspecs_count = 0;
	char delim[] = ",", *tmp_str, *chan_str;
	chanspec_t chan, chanspecs[WLDM_MAX_CH_LIST_LEN] = {0};
	uint8 control_channels[WLDM_MAX_CH_LIST_LEN] = {0};
	boolean incl_chanspecs[WLDM_MAX_CH_LIST_LEN] = {FALSE};
	char chanspec_str[STRING_LENGTH_16];
	boolean found, is_chanspec;

	if (wl_get_chanspecs_list(radioIndex, sizeof(chanspecs),
		chanspecs, control_channels, &chanspecs_count)) {
		WIFI_ERR("%s: Failed to get chanspecs\n", __FUNCTION__);
		return -1;
	}
	tmp_str = strdup(inp_channels);
	if (tmp_str == NULL) {
		WIFI_ERR("%s: Error strdup\n", __FUNCTION__);
		return -1;
	}

	chan_str = strtok(tmp_str, delim);
	memset(incl_chanspecs, FALSE, sizeof(incl_chanspecs));
	while (chan_str != NULL) {
		if (strstr(chan_str, "0x") != NULL) {
			chan = (chanspec_t)(strtoul(chan_str, NULL, 16));
			is_chanspec = TRUE;
		} else {
			chan = (chanspec_t)(strtoul(chan_str, NULL, 10));
			is_chanspec = FALSE;
		}
		found = FALSE;
		for (i = 0; i < chanspecs_count; i++) {
			if (((is_chanspec == FALSE) && (chan == control_channels[i])) ||
				((is_chanspec == TRUE) && (chan == chanspecs[i]))) {
				incl_chanspecs[i] = TRUE;
				found = TRUE;
			}
		}
		if (!found) {
			WIFI_ERR("%s: Error radioIndex=%d chan_str=%s\n",
				__FUNCTION__, radioIndex, chan_str);
			free(tmp_str);
			return -1;
		}
		chan_str = strtok(NULL, delim);
	} /* while */
	for (i = 0; i < chanspecs_count; i++) {
		if (incl_chanspecs[i] == FALSE) {
			snprintf(chanspec_str, sizeof(chanspec_str), "0x%x,", chanspecs[i]);
			if ((strlen(out_chanspecs) + strlen(chanspec_str)) > *chanspecs_len) {
				WIFI_ERR("%s: strlen(out_chanspecs + chanspec_str):%d > "
					"out_chanspecs size:%d\n",
					__FUNCTION__, strlen(out_chanspecs) + strlen(chanspec_str),
					*chanspecs_len);
				free(tmp_str);
				return -1;
			}
			strncat(out_chanspecs, chanspec_str, strlen(chanspec_str));
		}
	}

	*chanspecs_len = strlen(out_chanspecs);
	out_chanspecs[*chanspecs_len - 1] = '\0';
	free(tmp_str);
	return 0;
}

/* Validate each excluded chanspecs from nvram, then convert them to a list of included channels */
static int
wl_get_incl_channels(int radioIndex, char *inp_chanspecs, int chanspecs_len,
	char *out_channels, uint *channels_len)
{
	int i, j, chanspecs_count = 0;
	char delim[] = ",", *tmp_str, *chan_str, tmp_out_channels[BUF_SIZE];
	chanspec_t chanspec, chanspecs[WLDM_MAX_CH_LIST_LEN] = {0};
	uint8 control_channels[WLDM_MAX_CH_LIST_LEN] = {0};
	uint8 channels_count = 0, channels[WLDM_MAX_CH_LIST_LEN] = {0};
	boolean excl_chanspecs[WLDM_MAX_CH_LIST_LEN] = {FALSE};
	char channel_str[STRING_LENGTH_16];
	boolean found;

	if (wl_get_chanspecs_list(radioIndex, sizeof(chanspecs),
		chanspecs, control_channels, &chanspecs_count)) {
		WIFI_ERR("%s: Failed to get chanspecs\n", __FUNCTION__);
		return -1;
	}
	tmp_str = strdup(inp_chanspecs);
	if (tmp_str == NULL) {
		WIFI_ERR("%s: Error strdup\n", __FUNCTION__);
		return -1;
	}

	chan_str = strtok(tmp_str, delim);
	memset(excl_chanspecs, FALSE, sizeof(excl_chanspecs));
	memset(tmp_out_channels, 0, *channels_len);
	while (chan_str != NULL) {
		chanspec = (chanspec_t)(strtoul(chan_str, NULL, 16));
		for (i = 0; i < chanspecs_count; i++) {
			if (chanspec == chanspecs[i]) {
				excl_chanspecs[i] = TRUE;
				break;
			}
		}
		chan_str = strtok(NULL, delim);
	}

	found = FALSE;
	for (i = 0; i < chanspecs_count; i++) {
		if (excl_chanspecs[i] == FALSE) {
			for (j = 0; j < channels_count; j++) {
				if (channels[j] == control_channels[i]) {
					found  = TRUE;
					break;
				}
			}
			if (!found) {
				channels[channels_count] = control_channels[i];
				snprintf(channel_str, sizeof(channel_str), "%d,", channels[channels_count]);
				strncat(tmp_out_channels, channel_str, strlen(channel_str));
				channels_count++;
			}
		}
	}

	if (*channels_len < strlen(tmp_out_channels) + 1) {
		WIFI_ERR("%s: Error Buffer too short need %d got %d\n", __FUNCTION__,
			strlen(tmp_out_channels), *channels_len);
		free(tmp_str);
		return -1;
	}
	*channels_len = strlen(tmp_out_channels);
	tmp_out_channels[*channels_len - 1] = '\0';
	strcpy(out_channels, tmp_out_channels);
	free(tmp_str);
	return 0;
}

int
wldm_Radio_OperatingChannelBandwidth(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "OperatingChannelBandwidth";
	char *nvram_value = NULL, nvram_name[NVRAM_NAME_SIZE], *bw_str;
	int chanspec, i, bw_cap, band;
	unsignedInt current_bw;
	char *osifname, *nvifname, value[STRING_LENGTH_64] = {0}, retbuf[WLC_IOCTL_MEDLEN] = {0};
	struct {
		uint32 band;
		uint32 bw_cap;
	} bw_cap_param = { 0, 0 };

	osifname = wldm_get_radio_osifname(radioIndex);
	nvifname = wldm_get_radio_nvifname(radioIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "chanspec", &chanspec) < 0) {
			WIFI_ERR("%s: wl_iovar_getint(chanspec) failed!\n", __FUNCTION__);
			return -1;
		}
		current_bw = (chanspec & WL_CHANSPEC_BW_MASK);
		for (i = 0; i < (sizeof(cspec_bw) / sizeof(cspec_bw[0])); i++) {
			if (cspec_bw[i].bw == current_bw) {
				if (*plen <= strlen(cspec_bw[i].bw_str)) {
					WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
					return -1;
				}
				snprintf(pvalue, *plen, cspec_bw[i].bw_str);
				break;
			}
		}
		if (i >= (sizeof(cspec_bw) / sizeof(cspec_bw[0]))) {
			if (*plen <= strlen("N/A")) {
				WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
				return -1;
			}
			snprintf(pvalue, *plen, "N/A");
			WIFI_ERR("%s: Invalid current_bw=%d, chanspec=0x%04x\n",
				__FUNCTION__, current_bw, chanspec);
		}

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_bw_cap", nvifname);
		if ((nvram_value = wlcsm_nvram_get(nvram_name)) != NULL) {
			bw_cap = atoi(nvram_value);

			bw_str = (bw_cap == WLC_BW_CAP_160MHZ) ? "160MHz" :
				(bw_cap == WLC_BW_CAP_80MHZ) ? "80MHz" :
				(bw_cap == WLC_BW_CAP_40MHZ) ? "40MHz" : "20MHz";
			if (*plen <= strlen(bw_str)) {
				WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
				return -1;
			} else {
				snprintf(pvalue, *plen, bw_str);
			}
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n",
				__FUNCTION__, nvram_name);
			return -1;
		}
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex,
			Radio_OperatingChannelBandwidth_MASK);
		if (pObj == NULL)
			return -1;

		for (i = 0; i < ARRAY_SIZE(cspec_bw); i++) {
			if (!strcmp(cspec_bw[i].bw_str, pvalue)) {
				if (pObj->Radio.OperatingChannelBandwidth) {
					WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
						parameter, pObj->Radio.OperatingChannelBandwidth);
					free(pObj->Radio.OperatingChannelBandwidth);
				}
				pObj->Radio.OperatingChannelBandwidth =
					(char *) malloc(strlen(pvalue) + 1);
				if (pObj->Radio.OperatingChannelBandwidth == NULL) {
					WIFI_ERR("%s: malloc failure\n", __FUNCTION__);
					wldm_rel_Object(pObj, FALSE);
					return -1;
				}
				strcpy(pObj->Radio.OperatingChannelBandwidth, pvalue);
				pObj->apply_map |= Radio_OperatingChannelBandwidth_MASK;
				break;
			}
		}
		if (i >= ARRAY_SIZE(cspec_bw)) {
			WIFI_ERR("%s: Invalid OperatingChannelBandwidth %s\n",
				__FUNCTION__, pvalue);
			pObj->reject_map |= Radio_OperatingChannelBandwidth_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & (CMD_SET_NVRAM | CMD_SET_IOCTL)) {
		for (i = 0; i < ARRAY_SIZE(cspec_bw); i++) {
			if (!strcmp(cspec_bw[i].bw_str, pvalue)) {
				bw_cap = cspec_bw[i].bw_cap;
				snprintf(value, sizeof(value), "%d", bw_cap);
				break;
			}
		}
		if (i >= ARRAY_SIZE(cspec_bw)) {
			 WIFI_ERR("%s: Invalid OperatingChannelBandwidth %s\n",
						  __FUNCTION__, pvalue);
			return -1;
		}

		if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
			WIFI_ERR("%s: IOVAR to get band info failed\n", __FUNCTION__);
			return -1;
		}
		band = dtoh32(band);
		if ((band == WLC_BAND_2G) && (cspec_bw[i].bw >= WL_CHANSPEC_BW_80)) {
			WIFI_ERR("%s: %d invalid bw_cap for 2g band\n", __FUNCTION__, cspec_bw[i].bw);
			return -2;
		}

		if (cmd & CMD_SET_NVRAM) {
			snprintf(nvram_name, sizeof(nvram_name), "%s_bw_cap", nvifname);
			if (nvram_set(nvram_name, value) != 0) {
				WIFI_ERR("%s: nvram_set %s=%s failed!\n", __FUNCTION__,
					nvram_name, value);
				return -1;
			}
		}

		if (cmd & CMD_SET_IOCTL) {
			memset(retbuf, 0, sizeof(retbuf));
			bw_cap_param.band = band;
			bw_cap_param.bw_cap = bw_cap;
			if (wl_iovar_setbuf(osifname, "bw_cap", &bw_cap_param,
				sizeof(bw_cap_param), retbuf, WLC_IOCTL_MEDLEN) < 0) {
				WIFI_ERR("%s: wl_iovar_setbuf(bw_cap) failed!\n", __FUNCTION__);
				return -1;
			}
		}
	}

	return 0;
}

int
wldm_Radio_ExtensionChannel(int cmd, int radioIndex,
        string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ExtensionChannel";
	int chanspec, extensionChannel;
	char *osifname;

	osifname = wldm_get_radio_osifname(radioIndex);
	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_GET_NVRAM | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "chanspec", &chanspec) < 0) {
			WIFI_ERR("%s: wl_ioctl failed!\n", __FUNCTION__);
			return -1;
		}

		switch (extensionChannel = CHSPEC_CTL_SB(chanspec)) {
			case WL_CHANSPEC_CTL_SB_L:
				snprintf(pvalue, *plen, "BelowControlChannel");
				break;
			case WL_CHANSPEC_CTL_SB_U:
				snprintf(pvalue, *plen, "AboveControlChannel");
				break;
			default:
				snprintf(pvalue, *plen, "Auto");
				break;
		}

		*plen = strlen(pvalue) + 1;

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_ExtensionChannel_MASK);
		if (pObj == NULL)
			return -1;

		if ((strcmp(pvalue, "AboveControlChannel") == 0) ||
			(strcmp(pvalue, "BelowControlChannel") == 0) ||
			(strcmp(pvalue, "Auto") == 0)) {
			if (pObj->Radio.ExtensionChannel) {
				WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
					parameter, pObj->Radio.ExtensionChannel);
				free(pObj->Radio.ExtensionChannel);
			}
			pObj->Radio.ExtensionChannel = (char *) malloc(strlen(pvalue) + 1);
			if (pObj->Radio.ExtensionChannel == NULL) {
				WIFI_ERR("%s: malloc failure\n", __FUNCTION__);
				wldm_rel_Object(pObj, FALSE);
				return -1;
			}
			strcpy(pObj->Radio.ExtensionChannel, pvalue);
			pObj->apply_map |= Radio_ExtensionChannel_MASK;
		} else {
			WIFI_ERR("%s: Invalid RadioExtensionChannel %s\n",
				__FUNCTION__, pvalue);
			pObj->reject_map |= Radio_ExtensionChannel_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		wldm_rel_Object(pObj, TRUE);
	}

	return 0;
}

int
wldm_Radio_Channel(int cmd, int radioIndex,
	unsignedInt *pvalue, int *plen, int bw, int extChan, char *pbuf, int *pbufsz)
{
	char *parameter = "Channel";
	char *osifname, *nvifname;
	channel_info_t chanInfo;
	char chanspec_str[STRING_LENGTH_32];
	unsignedInt chanspec;
	char nvram_name[NVRAM_NAME_SIZE], nvram_value[STRING_LENGTH_64], *nvVal = NULL;
	char cmdBuf[BUF_SIZE] = {0};
	int pid;
	bool acs_boot_only = FALSE;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);
	nvifname = wldm_get_radio_nvifname(radioIndex);
	if (cmd & CMD_GET) {
		if (wl_ioctl(osifname, WLC_GET_CHANNEL, &chanInfo, sizeof(chanInfo)) < 0) {
			WIFI_ERR("%s: wl_ioctl() failed!\n", __FUNCTION__);
			return -1;
		}
		*pvalue = chanInfo.target_channel;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_Channel_MASK);
		if (pObj == NULL)
			return -1;

		if (wl_validate_channel(radioIndex, *pvalue) < 0) {
			pObj->reject_map |= Radio_Channel_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		pObj->Radio.Channel = *pvalue;
		pObj->apply_map |= Radio_Channel_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & (CMD_SET_NVRAM | CMD_SET_IOCTL)) {
		wl_gen_chanspec_str(radioIndex, *pvalue, chanspec_str,
			sizeof(chanspec_str), bw, extChan);
		chanspec = wf_chspec_aton(chanspec_str);
		WIFI_DBG("%s: Chanspec_str = %s, chanspec = 0x%04x for NVRAM/IOCTL\n",
				__FUNCTION__, chanspec_str, chanspec);
		if (wl_validate_chanspec(radioIndex, chanspec) < 0) {
			WIFI_ERR("%s: Wrong chanspec = 0x%04x for NVRAM/IOCTL\n",
				__FUNCTION__, chanspec);
			return -1;
		}
		if (cmd & CMD_SET_NVRAM) {
			snprintf(nvram_name, sizeof(nvram_name), "%s_channel", nvifname);
			snprintf(nvram_value, sizeof(nvram_value), "%d", *pvalue);
			if (wlcsm_nvram_set(nvram_name, nvram_value) != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
					nvram_name, nvram_value);
				return -1;
			};

			snprintf(nvram_name, sizeof(nvram_name), "%s_chanspec", nvifname);
			if (wlcsm_nvram_set(nvram_name, chanspec_str) != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
					nvram_name, chanspec_str);
				return -1;
			}
		}
		if (cmd & CMD_SET_IOCTL) {
			if (wl_iovar_setint(osifname, "chanspec", htod32(chanspec)) != 0) {
				WIFI_ERR("%s: Failed to set chanspec=%d via IOCTL\n",
					 __FUNCTION__, chanspec);
				return -1;
			}
			pid = get_pid_by_name(ACSD);
			if (pid <= 0) {
				WIFI_DBG("%s: acsd2 is not running\n", __FUNCTION__);
			} else {
				snprintf(nvram_name, sizeof(nvram_name), "%s_acs_boot_only",
					nvifname);
				nvVal = wlcsm_nvram_get(nvram_name);
				if (nvVal != NULL) {
					acs_boot_only = atoi(nvVal) ? TRUE : FALSE;
				}
				/* Set acsd2 disabled(acs_boot_only=1)) or
				 * monitor(acs_boot_only=0) mode
				 */
				snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s mode %d",
					ACS_CLI, osifname, acs_boot_only ? 0 : 1);
				if (system(cmdBuf) != 0) {
					WIFI_ERR("%s: syscmd failed for %s!\n",
						__FUNCTION__, cmdBuf);
					return -2;
				}
				WIFI_DBG("%s: %s:%d, set acsd2 %s mode\n", __FUNCTION__, nvram_name,
					acs_boot_only, (acs_boot_only) ? "disabled" : "monitor");
			}
		}
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_channel", nvifname);
		nvVal = wlcsm_nvram_get(nvram_name);
		if (!nvVal) {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}
		*pvalue = atoi(nvVal);
		*plen = sizeof(*pvalue);
	}

	return 0;
}

int
wldm_Radio_AutoChannelSupported(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AutoChannelSupported";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET |
		CMD_SET_NVRAM | CMD_GET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		*pvalue = TRUE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	return 0;
}

int
wldm_Radio_GuardInterval(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "GuardInterval";
	char *nvifname, *osifname, *psgi_tx;
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE];
	int sgi_tx = -1, i, tbl_size;
	wldm_value_set_t sgi_tx_tbl[] = {
		/* TR181,	nvram,	iovar */
		{"Auto",	"auto",	-1},
		{"400nsec",	"on",	1},
		{"800nsec",	"off",	0},
	};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	tbl_size = sizeof(sgi_tx_tbl) / sizeof(sgi_tx_tbl[0]);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "sgi_tx", &sgi_tx) < 0) {
			WIFI_ERR("%s: wl_iovar_getint() failed!\n", __FUNCTION__);
			return -1;
		}

		for (i = 0; i < tbl_size; i++) {
			if (sgi_tx_tbl[i].iovar == sgi_tx) {
				psgi_tx = sgi_tx_tbl[i].str_tr181;
				break;
			}
		}

		if (i >= tbl_size) {
			WIFI_ERR("%s: Invalid iovar return for sgi_tx=%d\n", __FUNCTION__, sgi_tx);
			return -3;
		}

		if (*plen < strlen(psgi_tx) + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, psgi_tx);
		*plen = strlen(pvalue) + 1;

		if (cmd & CMD_LIST) {
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
		}
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_GuardInterval_MASK);
		if (pObj == NULL)
			return -1;

		/* validate input value */
		for (i = 0; i < tbl_size; i++) {
			if (!strcasecmp(sgi_tx_tbl[i].str_tr181, pvalue)) {
				break;
			}
		}

		if (i >= tbl_size) {
			pObj->reject_map |= Radio_GuardInterval_MASK;
			WIFI_ERR("%s: %s is invalid input for guard interval\n", __FUNCTION__, pvalue);
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}

		if (pObj->Radio.GuardInterval) {
			WIFI_ERR("%s: free old %s value %s\n", __FUNCTION__, parameter, pObj->Radio.GuardInterval);
			free(pObj->Radio.GuardInterval);
		}

		pObj->Radio.GuardInterval = malloc(strlen(pvalue) + 1);
		if (pObj->Radio.GuardInterval == NULL) {
			pObj->reject_map |= Radio_GuardInterval_MASK;
			WIFI_ERR("%s: malloc failed!\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -3;
		}

		strcpy(pObj->Radio.GuardInterval, pvalue);
		pObj->apply_map |= Radio_GuardInterval_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_sgi_tx", nvifname);

		for (i = 0; i < tbl_size; i++) {
			if (!strcasecmp(sgi_tx_tbl[i].str_tr181, pvalue)) {
				psgi_tx = sgi_tx_tbl[i].str_nvram;
				break;
			}
		}

		if (i >= tbl_size) {
			WIFI_ERR("%s: %s is invalid input for %s\n", __FUNCTION__, pvalue, parameter);
			return -1;
		}

		NVRAM_SET(nvram_name, psgi_tx);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_sgi_tx", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value == NULL) {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}

		for (i = 0; i < tbl_size; i++) {
			if (!strcasecmp(sgi_tx_tbl[i].str_nvram, nvram_value)) {
				psgi_tx =  sgi_tx_tbl[i].str_tr181;
				break;
			}
		}

		if (i >= tbl_size) {
			WIFI_ERR("%s: %s is invalid nvram value for %s\n", __FUNCTION__, nvram_value, parameter);
			return -1;
		}

		if (*plen < strlen(psgi_tx) + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, psgi_tx);
		*plen = strlen(pvalue) + 1;
	}

	if (cmd & CMD_SET_IOCTL) {
		for (i = 0; i < tbl_size; i++) {
			if (!strcasecmp(sgi_tx_tbl[i].str_tr181, pvalue)) {
				sgi_tx = sgi_tx_tbl[i].iovar;
				break;
			}
		}

		if (i >= tbl_size) {
			WIFI_ERR("%s: %s is invalid %s\n", __FUNCTION__, pvalue, parameter);
			return -1;
		}

		IOVAR_INT_SET(osifname, "sgi_tx", sgi_tx);
	}

	return 0;
}

static char *
convert500KbpsRateToMbpsString(int rate_500k, char *rate_mbps)
{
	sprintf(rate_mbps, (rate_500k & 0x01) ? "%d.5" : "%d", rate_500k >> 1);
	return rate_mbps;
}

static int
parse_rateset(wl_rateset_t *rs, char *request, char *output, int len)
{
	uint32 i, count;
	char rate[8];
	int firstOne = 1, is_basic, is_supported;

	/* maximum rateset length is 12 */
	count = dtoh32(rs->count);
	if (count > MAX_NUM_RATE_SET)
		count = MAX_NUM_RATE_SET;

	memset(output, 0, len);

	is_basic = !strcmp(request, "basic");
	is_supported = !strcmp(request, "supported");
	if (!is_basic && !is_supported) {
		WIFI_ERR("%s: %s is invalid request\n", __FUNCTION__, request);
		return -1;
	}

	for (i = 0; i < count; i++) {
		memset(rate, 0, sizeof(rate));
		if (is_basic) {
			if (rs->rates[i] & 0x80) {
				if (firstOne) {
					firstOne = 0;
				} else {
					strcat(output, ",");
				}
				convert500KbpsRateToMbpsString((int)(rs->rates[i] & 0x7f), rate);
				strcat(output, rate);
			}
		} else if (is_supported) {
			if (firstOne) {
				firstOne = 0;
			} else {
				strcat(output, ",");
			}
			convert500KbpsRateToMbpsString((int)(rs->rates[i] & 0x7f), rate);
			strcat(output, rate);
		}
	}

	return 0;
}

static int
wl_get_current_rateset(char *osifname, char *request, char *rateset)
{
	int ret = -1;
	wl_rateset_t rs;
	char requestedRates[STRING_LENGTH_64];

	ret = wl_ioctl(osifname, WLC_GET_CURR_RATESET, &rs, sizeof(wl_rateset_t));
	if (ret < 0) {
		WIFI_ERR("%s: wl_iovar_get call for rateset fail\r\n", __FUNCTION__);
		ret = -2;
	} else {
		ret = parse_rateset(&rs, request, requestedRates, sizeof(requestedRates));
		if (ret == 0)
			strcpy(rateset, requestedRates);
	}

	return ret;
}

static int
get_rate_index(char *rate, char **rate_set_list, int rate_set_size, char *rate_str)
{
	int i;

	for (i = 0; i < rate_set_size; i++) {
		if (!strncmp(rate, rate_set_list[i], strlen(rate_set_list[i]) + 1))
			return i;
	}

	WIFI_ERR("%s: Invalid %s rate is %s\n", __FUNCTION__, rate_str, rate);
	return -1;
}

/* Per requirement on wifi_setApBeaconRate, user shall only select on of the following
 * rate as beacon rate
 * "1Mbps"; "5.5Mbps"; "6Mbps"; "2Mbps"; "11Mbps"; "12Mbps"; "24Mbps"
 * */
static int
isValidBeaconRate(char *rate)
{
	return get_rate_index(rate, ratesetBeacon, MAX_NUM_BEACON_RATE_SET, "Beacon");
}

static int
isOfdmRate(char *rate)
{
	return get_rate_index(rate, ratesetOFDM, MAX_NUM_OFDM_RATE_SET, "OFDM");
}

static int
validate_rateset(char *input, char *delimiters, struct d11_radioBasicRates_t *br, boolean *ofdmonly)
{
	char *tok, str[STRING_LENGTH_128];
	int i, idx;

	if (!input || (strlen(input) >= STRING_LENGTH_128)) {
		WIFI_ERR("%s: bad input\n", __FUNCTION__);
		return -1;
	}

	strcpy(str, input);
	tok = strtok(str, delimiters);
	*ofdmonly = TRUE;

	memset(br, 0, sizeof(*br));

	for (i = 0; tok && (i < MAX_NUM_BEACON_RATE_SET); i++) {
		if ((idx = isValidBeaconRate(tok)) < 0) {
			return -2;
		}

		if (isOfdmRate(tok) < 0) {
			*ofdmonly = FALSE;
		}

		br->basicRates[i] = ratesetBeacon[idx];
		tok = strtok(NULL, delimiters);
	}

	br->brlen = i;

	return 0;
}

static int
generate_rateset(char *basicRates, char *nv_rateset, char *ioctl_rateset)
{
	char **rates, *delimiter = " ,";
	int llen, foundMatchingRate = -1, i, j, foundBasicRate;
	boolean ofdmonly = FALSE;
	struct d11_radioBasicRates_t br;

	memset(&br, 0, sizeof(br));

	if (validate_rateset(basicRates, delimiter, &br, &ofdmonly) < 0) {
		WIFI_ERR("%s: failed to validate and convert to basic rate list\n", __FUNCTION__);
		return -1;
	}

	if (ofdmonly == TRUE) {
		WIFI_DBG("%s: basicRates are ofdm only\n", __FUNCTION__);
		rates = &ratesetOFDM[0];
		llen = MAX_NUM_OFDM_RATE_SET;
		strcpy(nv_rateset, "ofdmonly");
	} else {
		WIFI_DBG("%s: basicRates are not ofdm only\n", __FUNCTION__);
		rates = &ratesetALL[0];
		llen = MAX_NUM_RATE_SET;
		strcpy(nv_rateset, "default");
	}

	WIFI_DBG("%s: llen: %d\n", __FUNCTION__, llen);
	for (i = 0; i < llen; i++) {
		foundBasicRate = 0;
		strcat(ioctl_rateset, rates[i]);
		for (j = 0; j < br.brlen && br.basicRates[j]; j++) {
			if (!strcmp(rates[i], br.basicRates[j])) {
				foundBasicRate = 1;
				break;
			}
		}
		if (foundBasicRate) {
			strcat(ioctl_rateset, "b ");
			foundMatchingRate = 1;
		} else {
			strcat(ioctl_rateset, " ");
		}
	}

	return foundMatchingRate;
}

int
wldm_Radio_BasicDataTransmitRates(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "BasicDataTransmitRates";
	char *nvifname, *osifname, buf[BUF_SIZE];
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE];
	char rateset[STRING_LENGTH_64] = {0};
	char nv_rateset[STRING_LENGTH_64] = {0};
	int slen;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	/* for command set and get, the pvalue is string of rate seperated by comma, like 1,2,6,12 */
	if (cmd & CMD_GET) {
		if (wl_get_current_rateset(osifname, "basic", rateset) < 0) {
			WIFI_ERR("%s: wl_ioctl failed to get rateset!\n", __FUNCTION__);
			return -1;
		}

		if (*plen < strlen(rateset) + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}

		strcpy(pvalue, rateset);
		*plen = strlen(pvalue) + 1;

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_BasicDataTransmitRates_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: Invalid Radio_Object\n", __FUNCTION__);
			return -1;
		}

		if (pvalue == NULL) {
			WIFI_ERR("%s: Null pvalue\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}

		/* check the validation of the rates */
		if (generate_rateset(pvalue, nv_rateset, rateset) < 0) {
			pObj->reject_map |= Radio_BasicDataTransmitRates_MASK;
			WIFI_ERR("%s: invalid rateset %s.\n", __FUNCTION__, pvalue);
			wldm_rel_Object(pObj, FALSE);
			return -3;
		}

		if (pObj->Radio.BasicDataTransmitRates) {
			WIFI_ERR("%s: free old %s value %s\n",
				__FUNCTION__, parameter, pObj->Radio.BasicDataTransmitRates);
			free(pObj->Radio.BasicDataTransmitRates);
		}

		slen = strlen(pvalue) + 1;
		pObj->Radio.BasicDataTransmitRates = malloc(slen);
		if (pObj->Radio.BasicDataTransmitRates == NULL) {
			pObj->reject_map |= Radio_BasicDataTransmitRates_MASK;
			WIFI_ERR("%s: malloc failed!\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -4;
		}

		strcpy(pObj->Radio.BasicDataTransmitRates, pvalue);
		pObj->apply_map |= Radio_BasicDataTransmitRates_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	/* for nvram, the pvalue can only be "default" or "ofdmonly" */
	if (cmd & CMD_SET_NVRAM) {
		if (generate_rateset(pvalue, nv_rateset, rateset) < 0) {
			WIFI_ERR("%s: invalid rateset %s.\n", __FUNCTION__, pvalue);
			return -1;
		}
		snprintf(nvram_name, sizeof(nvram_name), "%s_rateset", nvifname);
		NVRAM_SET(nvram_name, nv_rateset);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_rateset", nvifname);
		if ((nvram_value = wlcsm_nvram_get(nvram_name))) {
			if (*plen < strlen(nvram_value) + 1) {
				WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
				return -1;
			}
			strcpy(pvalue, nvram_value);
			*plen = strlen(pvalue) + 1;
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		/* check the validation of the rates */
		memset(rateset, 0, STRING_LENGTH_64);
		if (generate_rateset(pvalue, nv_rateset, rateset) < 0) {
			WIFI_ERR("%s: invalid rateset %s.\n", __FUNCTION__, pvalue);
			return -1;
		}

		snprintf(buf, sizeof(buf), "wl -i %s rateset %s", osifname, rateset);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Coulnd't issue %s\n", __FUNCTION__, buf);
		}
	}

	return 0;
}

/**************************************************
* Device.WiFi.Radio.{i}.SupportedDataTransmitRates
***************************************************/
int
wldm_Radio_SupportedDataTransmitRates(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SupportedDataTransmitRates";
	char *osifname, rateset[STRING_LENGTH_64] = {0};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_GET_NVRAM |
		CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);
	if (cmd & CMD_GET) {
		if (wl_get_current_rateset(osifname, "supported", rateset) < 0) {
			WIFI_ERR("%s: wl_ioctl failed to get rateset!\n", __FUNCTION__);
			return -1;
		}

		if (*plen < strlen(rateset) + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, rateset);
		*plen = strlen(pvalue) + 1;

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	return 0;
}

int
wldm_Radio_OperationalDataTransmitRates(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "OperationalDataTransmitRates";
	char *osifname, rateset[STRING_LENGTH_64] = {0};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_GET_NVRAM |
		CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_get_current_rateset(osifname, "supported", rateset) < 0) {
			WIFI_ERR("%s: get_currect_rateset failed to get supported rateset!\n", __FUNCTION__);
			return -1;
		}

		if (*plen < strlen(rateset) + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, rateset);
		*plen = strlen(pvalue) + 1;

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	return 0;
}

int
wldm_xbrcm_Radio_CtsProtectionEnable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "CtsProtectionEnable";
	char nvram_name[NVRAM_NAME_SIZE], *nvVal;;
	char *nvifname, *osifname;
	int override, control;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "nmode_protection_override", &override) < 0) {
			WIFI_ERR("%s: wl_iovar_getint nmode_protection_override failed!\n", __FUNCTION__);
			return -1;
		}

		*pvalue = override ? TRUE : FALSE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
			X_BROADCOM_COM_Radio_CtsProtectionEnable_MASK);

		if (pObj == NULL)
			return -1;

		if ((*pvalue > 1) || (*pvalue < 0)) {
			pObj->reject_map |= X_BROADCOM_COM_Radio_CtsProtectionEnable_MASK;
			WIFI_ERR("%s: Invalid CtsProtection value %d\n", __FUNCTION__, *(int *)pvalue);
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}

		pObj->X_BROADCOM_COM_Radio.CtsProtectionEnable = *pvalue;
		pObj->apply_map |= X_BROADCOM_COM_Radio_CtsProtectionEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char *pnmode;

		snprintf(nvram_name, sizeof(nvram_name), "%s_nmode_protection", nvifname);

		pnmode = (*pvalue == TRUE) ? "auto" : "off";
		NVRAM_SET(nvram_name, pnmode);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_nmode_protection", nvifname);
		if ((nvVal = wlcsm_nvram_get(nvram_name))) {
			if (!strcmp(nvVal, "on") || !strcmp(nvVal, "auto"))
				*pvalue = TRUE;
			else if (!strcmp(nvVal, "off"))
				*pvalue = FALSE;
			else {
				WIFI_ERR("%s: invalid %s for %s\n", __FUNCTION__, nvVal, nvram_name);
				return -2;
			}

		} else {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}

		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_SET_IOCTL) {
		override = *pvalue ? WLC_PROTECTION_AUTO : WLC_PROTECTION_OFF;
		if (wl_iovar_setint(osifname, "nmode_protection_override", (uint32)override) < 0) {
			WIFI_ERR("%s: %s wl_iovar_setint nmode_protection_override to %d failed\n",
				       __FUNCTION__, osifname, override) ;
		}
		control = *pvalue ? WLC_PROTECTION_CTL_OVERLAP : WLC_PROTECTION_CTL_OFF;
		if (wl_ioctl(osifname, WLC_SET_PROTECTION_CONTROL, &control, sizeof(control)) < 0 ) {
			WIFI_ERR("%s: %s wl_ioctl set protection_control to %d failed\n",
					__FUNCTION__, osifname, control);
			return -4;
		}
	}

	return 0;
}

int
wldm_xbrcm_Radio_STBCEnable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "STBCEnable";
	char *nvifname, *osifname;
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE];
	int stbc_tx;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "stbc_tx", &stbc_tx) < 0) {
			WIFI_ERR("%s: wl_iovar_getint failed!\n", __FUNCTION__);
			return -1;
		}

		*pvalue = stbc_tx;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
			X_BROADCOM_COM_Radio_STBCEnable_MASK);
		if (pObj == NULL)
			return -1;

		if (*pvalue < -1 || *pvalue > 1) {
			pObj->reject_map |= X_BROADCOM_COM_Radio_STBCEnable_MASK;
			WIFI_ERR("%s: Invalid STBCEnable\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		pObj->X_BROADCOM_COM_Radio.STBCEnable = *pvalue;
		pObj->apply_map |= X_BROADCOM_COM_Radio_STBCEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char *ptr;

		snprintf(nvram_name, sizeof(nvram_name), "%s_stbc_tx", nvifname);
		if (*pvalue == 1 || *pvalue == -1)
			ptr = "auto";
		else if (*pvalue == 0)
			ptr = "off";
		else {
			WIFI_ERR("%s: invalid stbc_tx value %d\n",
				__FUNCTION__, *pvalue);
			return -1;
		}
		NVRAM_SET(nvram_name, ptr);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_stbc_tx", nvifname);

		if (!(nvram_value = wlcsm_nvram_get(nvram_name))) {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}

		if (!strcmp(nvram_value, "auto") || !strcmp(nvram_value, "on")) {
			*pvalue = TRUE;
		} else if (!strcmp(nvram_value, "off")) {
			*pvalue = FALSE;
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get return invalid %s!\n",
				__FUNCTION__, nvram_value);
			return -2;
		}

		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_SET_IOCTL) {
		/* example from wifi_hal.c
		 * Set sTBC mode. The mode is enumeration of: auto=-1, off=0, on=1
		 * sTBCMode = STBC_Enable ? -1 : 0;
		 */
		stbc_tx = (*pvalue == TRUE) ? -1 : 0 ;
		IOVAR_INT_SET(osifname, "stbc_tx", stbc_tx);
	}

	return 0;
}

static int
mask_to_count(int mask)
{
	int count, tmp = mask & ANTENNA_NUM_MASK;
	for (count = 0; tmp; tmp >>= 1, count++);
	return count;
}

static int
count_to_mask(int count)
{
	return ((1 << count) - 1);
}

int
wldm_xbrcm_Radio_TxChainMask(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "TxChainMask";
	char *nvifname, *osifname;
	char nvram_name[NVRAM_NAME_SIZE], *nvram_value;
	int mask = 0;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "txchain", &mask) < 0) {
			WIFI_ERR("%s: wl_iovar_getint failed!\n", __FUNCTION__);
			return -1;
		}

		*pvalue = mask_to_count(mask);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
			X_BROADCOM_COM_Radio_TxChainMask_MASK);

		if (pObj == NULL) {
			WIFI_ERR("%s: pObj is null\n", __FUNCTION__);
			return -1;
		}

		/* validate tx chain */
		if (*pvalue < 1 || *pvalue > MAX_NUM_ANTENNA_CHAIN) {
			pObj->reject_map |= X_BROADCOM_COM_Radio_TxChainMask_MASK;
			WIFI_ERR("%s: Invalid TxChainMask %d\n", __FUNCTION__, *pvalue);
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}

		pObj->X_BROADCOM_COM_Radio.TxChainMask = *pvalue;
		pObj->apply_map |= X_BROADCOM_COM_Radio_TxChainMask_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		mask = count_to_mask(*pvalue);
		NVRAM_INT_SET(nvifname, "txchain", mask);
		NVRAM_INT_SET(nvifname, "hw_txchain", mask);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_txchain", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value) {
			*pvalue = mask_to_count(atoi(nvram_value));
			*plen = sizeof(*pvalue);
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		if (*pvalue < 1 || *pvalue > 4) {
			WIFI_ERR("%s: Invalid TxChainMask\n", __FUNCTION__);
			return -1;
		}

		mask = count_to_mask(*pvalue);
		IOVAR_INT_SET(osifname, "txchain", mask);
		/* "hw_txchain" not supported on 43684 */
	}

	return 0;
}

int
wldm_xbrcm_Radio_RxChainMask(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RxChainMask";
	char *nvifname, *osifname, nvram_name[NVRAM_NAME_SIZE], *nvVal;
	int mask = 0;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_rxchain", nvifname);
		if ((nvVal = wlcsm_nvram_get(nvram_name))) {
			mask = atoi(nvVal);
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}

		*pvalue = mask_to_count(mask);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);

	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
			X_BROADCOM_COM_Radio_RxChainMask_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: pObj is null\n", __FUNCTION__);
			return -1;
		}

		/* validate rxchain */
		if (*pvalue < 1 || *pvalue > MAX_NUM_ANTENNA_CHAIN) {
			pObj->reject_map |= X_BROADCOM_COM_Radio_RxChainMask_MASK;
			WIFI_ERR("%s: Invalid RxChainMask\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}

		pObj->X_BROADCOM_COM_Radio.RxChainMask = *pvalue;
		pObj->apply_map |= X_BROADCOM_COM_Radio_RxChainMask_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		mask = count_to_mask(*pvalue);
		NVRAM_INT_SET(nvifname, "rxchain", mask);
		NVRAM_INT_SET(nvifname, "hw_rxchain", mask);
	}

	if (cmd & CMD_SET_IOCTL) {
		int mask;

		if (*pvalue < 1 || *pvalue > 4) {
			WIFI_ERR("%s: Invalid RxChainMask\n", __FUNCTION__);
			return -1;
		}

		mask = count_to_mask(*pvalue);
		IOVAR_INT_SET(osifname, "rxchain", mask);

		/* "hw_txchain" not supported on 43684 */
	}

	return 0;
}

/* Function to call the ioctl for scan */
static int
wl_do_scan(char *osifname, wl_scan_params_t *scan_params, uint scan_params_size)
{
	if (wl_ioctl(osifname, WLC_SCAN, scan_params, scan_params_size) < 0) {
		WIFI_ERR("Err %s:%d ioctl for scan %s failed\n", __FUNCTION__, __LINE__, osifname);
		return -1;
	}
	return 0;
}

/* Function to call the ioctl for scan results */
static int
wl_get_scan_results(char *osifname, wl_scan_results_t *scan_results, uint scan_results_size)
{
	scan_results->buflen = htod32(scan_results_size);
	if (wl_ioctl(osifname, WLC_SCAN_RESULTS, scan_results, scan_results_size) < 0) {
		WIFI_ERR("Err %s:%d ioctl for scan results %s failed\n", __FUNCTION__, __LINE__,
			osifname);
		return -1;
	}
	scan_results->buflen = dtoh32(scan_results->buflen);
	scan_results->version = dtoh32(scan_results->version);
	scan_results->count = dtoh32(scan_results->count);
	if (scan_results->buflen == 0) {
		scan_results->version = 0;
		scan_results->count = 0;
		return -1;
	} else if (scan_results->version != WL_BSS_INFO_VERSION &&
		scan_results->version != LEGACY2_WL_BSS_INFO_VERSION &&
		scan_results->version != LEGACY_WL_BSS_INFO_VERSION) {
		WIFI_ERR("Err %s:%d driver has unsupported bss_info_version %d\n", __FUNCTION__,
			__LINE__, scan_results->version);
		scan_results->buflen = 0;
		scan_results->count = 0;
		return -1;
	}
	return 0;
}

/* Function to get the chanim_stats related HAL info */
/* For channel_stats/channel_stats2 *plen = # of channels that need the stats */
static int
dm_chanstats(int cmd, int index, void *pvalue, int *plen, char *pvar)
{
	char *osifname = wldm_get_radio_osifname(index), *pioctl_buf;
	int len = WLC_IOCTL_MAXLEN, txop = 0, i, j;
	uint stats_len = sizeof(wl_chanim_stats_t), stats_count = 1;
	wl_chanim_stats_t *chanim_ptr;

	pioctl_buf = malloc(len);
	if (pioctl_buf == NULL) {
		WIFI_ERR("%s: out of memory for %s!\n", __FUNCTION__, pvar);
		return -1;
	}
	if (cmd & CMD_GET) {
		chanim_ptr = (wl_chanim_stats_t *)pioctl_buf;
		/* Change the stats len and count only for channel_stats */
		if (strcmp(pvar, "channel_stats") == 0) {
			stats_len = WL_CHANIM_BUF_LEN;
			stats_count = WL_CHANIM_COUNT_ALL;
		}

		if (wl_get_chanim_stats(osifname, stats_len, stats_count, pioctl_buf, len) < 0) {
			WIFI_ERR("Err: %s:%d %s wl_get_chanim error\n", __FUNCTION__, __LINE__,
				osifname);
			free(pioctl_buf);
			return -1;
		}

		chanim_ptr->version = dtoh32(chanim_ptr->version);
		chanim_ptr->count = dtoh32(chanim_ptr->count);
		if (chanim_ptr->version < WL_CHANIM_STATS_VERSION_2) {
			WIFI_ERR("Err: chanim_stats version %d is too low\n",
				chanim_ptr->version);
			free(pioctl_buf);
			return -1;
		}

		if (strcmp(pvar, "bw_util") == 0) {
			if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_2) {
				chanim_stats_v2_t *pStats_v2 = (chanim_stats_v2_t *)chanim_ptr->stats;

				txop = pStats_v2->ccastats[CCASTATS_TXOP];
				txop = (txop > 100) ? 100 : txop;
				*(int *)pvalue = 100 - txop;
			} else if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_3) {
				chanim_stats_v3_t *pStats_v3 = (chanim_stats_v3_t *)chanim_ptr->stats;

				txop = pStats_v3->ccastats[CCASTATS_TXOP];
				txop = (txop > 100) ? 100 : txop;
				*(int *)pvalue = 100 - txop;
			} else {
				*(int *)pvalue = chanim_ptr->stats->channel_utilization;
			}
		} else if (strcmp(pvar, "channel_stats") == 0) {
			boolean found_match;
			wldm_channel_stats_t *pchan_stats = (wldm_channel_stats_t *)pvalue;
			wl_chanim_stats_us_v2_t *chanim_us_ptr;

			for (i = 0; i < chanim_ptr->count; i++) {
				for (j = 0; j < *plen; ++j) {
					if (CHSPEC_CHANNEL(chanim_ptr->stats[i].chanspec) ==
						pchan_stats[j].ch_number) {
						pchan_stats[j].ch_noise =
							chanim_ptr->stats[i].bgnoise;
						pchan_stats[j].ch_radar_noise = 0; /* TODO */
						pchan_stats[j].ch_non_80211_noise = 0; /* TODO */
						pchan_stats[j].ch_max_80211_rssi = 0; /* TODO */

						if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_2) {
							chanim_stats_v2_t *pStats_v2 = (chanim_stats_v2_t *)&(chanim_ptr->stats[i]);

							txop = pStats_v2->ccastats[CCASTATS_TXOP];
							txop = (txop > 100) ? 100 : txop;
							pchan_stats[j].ch_utilization = 100 - txop;
						} else if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_3) {
							chanim_stats_v3_t *pStats_v3 = (chanim_stats_v3_t *)&(chanim_ptr->stats[i]);

							txop = pStats_v3->ccastats[CCASTATS_TXOP];
							txop = (txop > 100) ? 100 : txop;
							pchan_stats[j].ch_utilization = 100 - txop;
						} else {
							chanim_stats_t *pStats = &chanim_ptr->stats[i];
							pchan_stats[j].ch_utilization = pStats->channel_utilization;
						}

						found_match = 1;
						break;
					}
				}
			}
			if (!found_match) {
				WIFI_ERR("Err: %s:%d Channel not found in the scan \n",
					__FUNCTION__, __LINE__);
				free(pioctl_buf);
				return -1;
			}
			stats_count = WL_CHANIM_COUNT_US_ALL;
			if (wl_get_chanim_stats(osifname, stats_len, stats_count, pioctl_buf, len)
				< 0) {
				WIFI_ERR("Err: %s:%d %s wl_get_chanim error\n", __FUNCTION__,
					__LINE__, osifname);
				free(pioctl_buf);
				return -1;
			}
			chanim_us_ptr = (wl_chanim_stats_us_v2_t *) pioctl_buf;

			for (i = 0; i < chanim_us_ptr->count; i++) {
				for (j = 0; j < *plen; ++j) {
					if (CHSPEC_CHANNEL(chanim_us_ptr->stats_us_v2[i].chanspec) ==
						pchan_stats[j].ch_number) {
						pchan_stats[j].ch_utilization_total =
							dtoh64(chanim_us_ptr->stats_us_v2[i].total_tm);
						pchan_stats[j].ch_utilization_busy =
							dtoh64(chanim_us_ptr->stats_us_v2[i].busy_tm);
						pchan_stats[j].ch_utilization_busy_tx =
							dtoh64(chanim_us_ptr->stats_us_v2[i].ccastats_us[CCASTATS_TXDUR]);
						pchan_stats[j].ch_utilization_busy_rx =
							dtoh64(chanim_us_ptr->stats_us_v2[i].ccastats_us[CCASTATS_OBSS]) +
							dtoh64(chanim_us_ptr->stats_us_v2[i].ccastats_us[CCASTATS_INBSS]) +
							dtoh64(chanim_us_ptr->stats_us_v2[i].ccastats_us[CCASTATS_NOPKT]) +
							dtoh64(chanim_us_ptr->stats_us_v2[i].ccastats_us[CCASTATS_NOCTG]);
						pchan_stats[j].ch_utilization_busy_self =
							dtoh64(chanim_us_ptr->stats_us_v2[i].ccastats_us[CCASTATS_INBSS]);
						pchan_stats[j].ch_utilization_busy_ext =
							dtoh64(chanim_us_ptr->stats_us_v2[i].rxcrs_sec20);
						found_match = 1;
						break;
					}
				}
			}
			if (!found_match) {
				WIFI_ERR("Err: %s:%d Channel not found in the scan \n",
					__FUNCTION__, __LINE__);
				free(pioctl_buf);
				return -1;
			}
		} else if (strcmp(pvar, "channel_stats2") == 0) {
			int rssi = 0, chanspec;
			unsigned int scan_params_size = WL_SCAN_PARAMS_FIXED_SIZE + sizeof(uint16);
			wldm_channel_stats2_t *pchan2_stats = (wldm_channel_stats2_t *)pvalue;
			wl_scan_params_t *scan_params;
			wl_scan_results_t *scan_results;
			wl_bss_info_t *bi;

			scan_params = (wl_scan_params_t *)malloc(scan_params_size);
			if (scan_params == NULL) {
				WIFI_ERR("%s:%d malloc failed!\n", __FUNCTION__, __LINE__);
				free(pioctl_buf);
				return -1;
			}
			if (wl_iovar_getint(osifname, "chanspec", &chanspec) < 0) {
				WIFI_DBG("%s: wl_iovar_getint(chanspec) failed!\n", __FUNCTION__);
				free(pioctl_buf);
				free(scan_params);
				return -1;
			}

			memset(scan_params, 0, scan_params_size);
			scan_params->bss_type = DOT11_BSSTYPE_ANY;
			memcpy(&scan_params->bssid, &ether_bcast, ETHER_ADDR_LEN);
			scan_params->scan_type = -1;
			scan_params->nprobes = -1;
			scan_params->active_time = -1;
			scan_params->passive_time = -1;
			scan_params->home_time = -1;
			scan_params->channel_num = 1;
			scan_params->channel_list[0] = chanspec;

			scan_results = (wl_scan_results_t *)malloc(DCS_SCANRESULTS_BUF_LEN);
			if (scan_results == NULL) {
				WIFI_ERR("%s:%d malloc failed!\n", __FUNCTION__, __LINE__);
				free(pioctl_buf);
				free(scan_params);
				return -1;
			}
			if (wl_do_scan(osifname, scan_params, scan_params_size) == 0) {
				sleep(2); /* sleep to get the scan results */
				if (wl_get_scan_results(osifname, scan_results,
					DCS_SCANRESULTS_BUF_LEN) == 0) {
					bi = scan_results->bss_info;
					rssi = bi->RSSI;
					for (i = 0; i < scan_results->count; i++,
						bi = (wl_bss_info_t *)((int8 *)bi + dtoh32(bi->length))) {
						if (rssi < bi->RSSI) {
							rssi = bi->RSSI;
						}
					}
				}
			}
			pchan2_stats->ch_Frequency = wf_chspec_ctlchan(chanspec);
			if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_2) {
				chanim_stats_v2_t *pStats_v2 = (chanim_stats_v2_t *)chanim_ptr->stats;

				pchan2_stats->ch_ObssUtil = pStats_v2->ccastats[CCASTATS_OBSS];
				pchan2_stats->ch_SelfBssUtil = pStats_v2->ccastats[CCASTATS_INBSS];
				pchan2_stats->ch_Non80211Noise = pStats_v2->bgnoise;
			} else {
				chanim_stats_t *pStats = (chanim_stats_t *)chanim_ptr->stats;

				pchan2_stats->ch_ObssUtil = pStats->ccastats[CCASTATS_OBSS];
				pchan2_stats->ch_SelfBssUtil = pStats->ccastats[CCASTATS_INBSS];
				pchan2_stats->ch_Non80211Noise = pStats->bgnoise;
			}
			pchan2_stats->ch_Max80211Rssi = rssi;
			free(scan_params);
			free(scan_results);
		}
	}
	free(pioctl_buf);
	return 0;
}

#define PHY_TYPE_AC             11	/* from d11.h */

static int
wl_phytype(char *osifname, uint *phytype)
{
        if (wl_ioctl(osifname, WLC_GET_PHYTYPE, phytype, sizeof(uint)) < 0) {
                WIFI_ERR("%s: failed to read back PHY TYPE\n", __FUNCTION__);
                return -1;
        }

        return 0;
}

int
wldm_xbrcm_Radio_Greenfield11nSupported(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Greenfield11nSupported";
        char *osifname;
        uint phytype;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_SET_NVRAM |
				CMD_GET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

        osifname = wldm_get_radio_osifname(radioIndex);

        if (wl_phytype(osifname, &phytype) < 0) {
                WIFI_ERR("%s: failed to read phytype from %s\n", __FUNCTION__, osifname);
                return -1;
        }

	/* always False for Broadcom wifi 802.11ac or later chip set */
	if (cmd & CMD_GET) {
		*pvalue = (phytype == PHY_TYPE_AC) ? FALSE : TRUE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	return 0;
}

int
wldm_xbrcm_Radio_Greenfield11nEnable(int cmd, int radioIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Greenfield11nEnable";
	char *nvifname, *osifname, *ptr;
	char *nvram_value,  nvram_name[NVRAM_NAME_SIZE];
	int val;
        uint phytype;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
                PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
                return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

        if (wl_phytype(osifname, &phytype) < 0) {
                WIFI_ERR("%s: failed to read phytype from %s\n", __FUNCTION__, osifname);
                return -1;
        }

	if (cmd & CMD_GET)  {
                if (phytype == PHY_TYPE_AC) {
                        *pvalue = FALSE;
                } else {
                        if (wl_iovar_getint(osifname, "mimo_preamble", &val) < 0) {
                                WIFI_ERR("%s: wl_iovar_getint failed!\n", __FUNCTION__);
                                return -2;
                        }
                        *pvalue = (val == 1) ? TRUE : FALSE ;
                }
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_GET_NVRAM)  {
		snprintf(nvram_name, sizeof(nvram_name), "%s_mimo_preamble", nvifname);
		nvram_value = nvram_safe_get(nvram_name);

		*pvalue = (strcmp(nvram_value, "gf") == 0) ? TRUE : FALSE;
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
			X_BROADCOM_COM_Radio_Greenfield11nEnable_MASK);

		if (pObj == NULL)
			return -1;

		if ((phytype == PHY_TYPE_AC) && *pvalue) {
			WIFI_ERR("%s: invalid %d for AC PHY", __FUNCTION__, *pvalue);
			pObj->reject_map |= X_BROADCOM_COM_Radio_Greenfield11nEnable_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}

		pObj->X_BROADCOM_COM_Radio.Greenfield11nEnable = *pvalue;
		pObj->apply_map |= X_BROADCOM_COM_Radio_Greenfield11nEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		/* drive default is mixed mode. Only 11n PHY supports greenfield */
                val = (phytype == PHY_TYPE_AC) ? 0 : (*pvalue ? 1 : 0);

		snprintf(nvram_name, sizeof(nvram_name), "%s_mimo_preamble", nvifname);
		ptr = (val == 0) ? "mm" : (val == 1) ? "gf" : "auto";

		NVRAM_SET(nvram_name, ptr);
	}

	if (cmd & CMD_SET_IOCTL) {
                val = (phytype == PHY_TYPE_AC) ? 0 : (*pvalue ? 1 : 0);
                IOVAR_INT_SET(osifname, "mimo_preamble", val);
	}

	return 0;
}

int
wldm_xbrcm_Radio_ResetCount(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ResetCount";
	char *nvifname;
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM))  {
		snprintf(nvram_name, sizeof(nvram_name), "%s_reset_count", nvifname);

		nvram_value = wlcsm_nvram_get(nvram_name);
		*pvalue = (!nvram_value) ? 0 : atoi(nvram_value);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

/* Assumes that dfs_chs_status_list is partially set from reading per_chan_info IOVAR */
static int
wl_set_dfs_ch_status(unsigned long channel, uint chspec_bw, uint num_dfs_channels,
	uint32 dfs_cac_state, wldm_dfs_status_t *dfs_chs_status_list)
{
	int i;

	if (!channel) {
		/* Channel can be zero, for example, any ext80[4] in 80MHz channel */
		WIFI_DBG("%s: Sub band channel %lu not defined\n", __FUNCTION__, channel);
		return 0;
	}
	if (dfs_cac_state == WL_DFS_CACSTATE_IDLE) {
		/* OK if operating in non-radar channel */
		return 0;
	}
	for (i = 0; i < num_dfs_channels; i++) {
		if (channel == dfs_chs_status_list[i].channel) {
			dfs_chs_status_list[i].status = (dfs_cac_state == WL_DFS_CACSTATE_POSTISM_OOC) ?
				WLDM_DFS_NOT_CLEARED :
				(dfs_cac_state == WL_DFS_CACSTATE_PREISM_OOC) ?
				WLDM_DFS_NOT_CLEARED :
				(dfs_cac_state == WL_DFS_CACSTATE_POSTISM_CAC) ?
				WLDM_DFS_CLEARED :
				(dfs_cac_state == WL_DFS_CACSTATE_PREISM_CAC) ?
				WLDM_DFS_CAC_PERIOD :
				(dfs_cac_state == WL_DFS_CACSTATE_ISM) ?
				WLDM_DFS_CLEARED :
				WLDM_DFS_NOT_CLEARED;
			return 0;
		}
	}
	if (chspec_bw == WL_CHANSPEC_BW_160) {
		/* All 160MHz channels are DFS, so OK if subband is non-radar channel */
		return 0;
	}
	/* Log unexpected channel number */
	WIFI_ERR("%s: Channel %lu not found in dfs_chs_status_list\n", __FUNCTION__, channel);
	return -1;
}

static int
dm_dfs_num_chs(int cmd, int index, uint *pvalue, uint *plen, char *pvar)
{
	char *osifname = wldm_get_radio_osifname(index);
	wl_uint32_list_t *list;
	uint channels_buf[WL_NUMCHANNELS + 1] = {0}, num_dfs_channels = 0;
	int wl_band, i;
	char chan_info_buf[BUF_SIZE] = {0};
	uint32 chspec_info = 0;

	if (strcmp(pvar, "dfs_num_chs") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;
	}

	if (wl_get_wl_band(osifname, &wl_band) < 0) {
		WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
		return -1;
	}

	if (cmd & CMD_GET) {
		*plen = sizeof(*pvalue);
		if (wl_band != WLC_BAND_5G) {
			WIFI_DBG("%s: %d is not supported band\n", __FUNCTION__, wl_band);
			*pvalue = 0;
			return 0;
		}

		list = (wl_uint32_list_t *)channels_buf;
		list->count = htod32(WL_NUMCHANNELS);
		if (wl_ioctl(osifname, WLC_GET_VALID_CHANNELS, channels_buf, sizeof(channels_buf)) < 0) {
			WIFI_ERR("%s: IOVAR to get valid channels failed\n", __FUNCTION__);
			return -1;
		}
		list->count = dtoh32(list->count);
		if (!list->count) {
			WIFI_ERR("%s: number of valid channels is 0\n", __FUNCTION__);
			return -1;
		}

		/* Get num DFS channels */
		for (i = 0; i < list->count; i++) {
			if (wl_get_per_chan_info(osifname, (uint) list->element[i], chan_info_buf,
				BUF_SIZE) < 0) {
				WIFI_ERR("%s: IOVAR to get per chan info failed\n", __FUNCTION__);
				return -1;
			}
			chspec_info = dtoh32(*(uint32 *)chan_info_buf);
			if (chspec_info & WL_CHAN_RADAR) {
				num_dfs_channels++;
			}
		}
		*pvalue = num_dfs_channels;
	}

	return 0;
}

static int
dm_dfs_chs_status(int cmd, int index, uint *pvalue, uint *plen, char *pvar)
{
	char *osifname = wldm_get_radio_osifname(index);
	wl_uint32_list_t *list;
	uint channels_buf[WL_NUMCHANNELS + 1] = {0}, num_dfs_channels = *plen, chspec_bw = 0;
	int wl_band, i, j;
	char chan_info_buf[BUF_SIZE] = {0}, dfs_status_all_buf[BUF_SIZE] = {0};
	uint32 chspec_info = 0;
	wldm_dfs_status_t *dfs_chs_status_list = (wldm_dfs_status_t *) pvalue;
	wl_dfs_status_all_t *dfs_status_all_list = NULL;
	wl_dfs_sub_status_t *sub = NULL;
	wldm_channel_t chan;
	uint8 *subband;

	if (strcmp(pvar, "dfs_chs_status") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;
	}

	if (wl_get_wl_band(osifname, &wl_band) < 0) {
		WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
		return -2;
	}

	if (cmd & CMD_GET) {
		if (wl_band != WLC_BAND_5G) {
			WIFI_DBG("%s: %d is not supported band\n", __FUNCTION__, wl_band);
			*pvalue = 0;
			*plen = 0;
			return 0;
		}

		list = (wl_uint32_list_t *)channels_buf;
		list->count = htod32(WL_NUMCHANNELS);
		if (wl_ioctl(osifname, WLC_GET_VALID_CHANNELS, channels_buf, sizeof(channels_buf)) < 0) {
			WIFI_ERR("%s: IOVAR to get valid channels failed\n", __FUNCTION__);
			return -3;
		}
		list->count = dtoh32(list->count);
		if (!list->count) {
			WIFI_ERR("%s: number of valid channels is 0\n", __FUNCTION__);
			return -4;
		}

		/* Set DFS channel status as one of the following: in CAC, cleared or not cleared */
		j = 0;
		for (i = 0; i < list->count; i++) {
			if (wl_get_per_chan_info(osifname, (uint) list->element[i], chan_info_buf,
				BUF_SIZE) < 0) {
				WIFI_ERR("%s: IOVAR to get per chan info failed\n", __FUNCTION__);
				return -5;
			}
			chspec_info = dtoh32(*(uint32 *)chan_info_buf);
			if (chspec_info & WL_CHAN_RADAR) {
				if (j == num_dfs_channels) {
					WIFI_ERR("%s: allocated < actual num DFS channels %d < %d\n",
						__FUNCTION__, num_dfs_channels, j + 1);
					return -6;
				}
				if (chspec_info & WL_CHAN_INACTIVE) {
					dfs_chs_status_list[j].status = WLDM_DFS_NOT_CLEARED;
				} else if (!(chspec_info & WL_CHAN_PASSIVE)) {
					dfs_chs_status_list[j].status = WLDM_DFS_CLEARED;
				} else if (chspec_info & WL_CHAN_PASSIVE) {
					dfs_chs_status_list[j].status = WLDM_DFS_NOT_CLEARED;
				}
				dfs_chs_status_list[j].channel = (unsigned long) list->element[i];
				j++;
			}
		}
		*plen = j;
		memset(dfs_status_all_buf, 0, BUF_SIZE);
		dfs_status_all_list = (wl_dfs_status_all_t *) dfs_status_all_buf;
		if (wl_get_dfs_status_all(osifname, dfs_status_all_list, BUF_SIZE) < 0) {
			WIFI_ERR("%s: IOVAR to get dfs status all failed\n", __FUNCTION__);
			return -7;
		}

		for (i = 0; i < dfs_status_all_list->num_sub_status; ++i) {
			sub = &dfs_status_all_list->dfs_sub_status[i];
			if (!sub->chanspec) {
				continue;
			}
			memset(&chan, 0, sizeof(wldm_channel_t));
			wl_parse_chanspec(sub->chanspec, &chan);
			/* Set DFS channel status from wl dfs_status_all */
			subband = (uint8 *) &chan;
			chspec_bw = sub->chanspec & WL_CHANSPEC_BW_MASK;
			for (j = 0; j < sizeof(chan); j++) {
				if (wl_set_dfs_ch_status((unsigned long) subband[j], chspec_bw,
					num_dfs_channels, sub->state, dfs_chs_status_list) < 0) {
					WIFI_ERR("%s: Error in set dfs ch status channel=%d\n",
					__FUNCTION__, subband[j]);
					return -8;
				}
			}
		}
	}

	return 0;
}

static xbrcm_t xbrcm_11h_dfs_tbl[] = {
	{  "csa_bcast",		{dm_csa},		CMD_SET_IOCTL,			},
	{  "dfs_num_chs",	{dm_dfs_num_chs},	CMD_GET,			},
	{  "dfs_chs_status",	{dm_dfs_chs_status},	CMD_GET,			},
	{  NULL,		{NULL},			0,				},
};

int
wldm_11h_dfs(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);

	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_11h_dfs_tbl);
}
static int
wl_deauth(int apIndex, struct ether_addr *eaPtr)
{
	char *osifname;
	scb_val_t scb_val;
	int ret;

	osifname = wldm_get_osifname(apIndex);
	memset(&scb_val, 0, sizeof(scb_val));
	memcpy(&scb_val.ea, eaPtr, ETHER_ADDR_LEN);
	WIFI_DBG("%s apIndex %d deauth %s\n", __FUNCTION__, apIndex, wl_ether_etoa(&scb_val.ea));
	ret = wl_ioctl(osifname, WLC_SCB_DEAUTHENTICATE, (void *)&scb_val.ea, ETHER_ADDR_LEN);
	if (ret < 0) {
		WIFI_ERR("%s: apIndex %d Error deauthenticating ret = %d\n", __FUNCTION__, apIndex, ret);
		return -1;
	}
	return 0;
}

/* Send Channel Switch Announcement */
static int
dm_csa(int cmd, int radioIndex, void *pvalue, uint *plen, char *pvar)
{
	wl_chan_switch_t wl_csa;
	csa_t *pcsa = (csa_t *)pvalue;
	char chanspec_str[STRING_LENGTH_32], nvram_name[NVRAM_NAME_SIZE], *nvram_value;
	int ret, csa_deauth_mode = 0;
	char *osifname = wldm_get_radio_osifname(radioIndex);
	char *nvifname = wldm_get_radio_nvifname(radioIndex);

	if (strcmp(pvar, "csa_bcast") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;
	}

	if (cmd & CMD_SET_IOCTL) {
		memset(chanspec_str, 0, sizeof(chanspec_str));
		wl_gen_chanspec_str(radioIndex, pcsa->channel, chanspec_str,
			sizeof(chanspec_str), pcsa->channel_width, 0);
		snprintf(nvram_name, sizeof(nvram_name), "%s_csa_deauth_mode", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		csa_deauth_mode = (!nvram_value) ? 0 : atoi(nvram_value);

		if (csa_deauth_mode != 0) {
			/* Deauth all associated clients */
			if ((ret = wl_deauth(radioIndex, (struct ether_addr *)&ether_bcast)) < 0) {
				WIFI_ERR("%s: radioIndex %d Error deauthenticating, ret = %d\n", __FUNCTION__, radioIndex, ret);
				return -1;
			}
		}
		wl_csa.mode = DOT11_CSA_MODE_ADVISORY;
		wl_csa.count = pcsa->csa_beacon_count;
		wl_csa.chspec = wf_chspec_aton(chanspec_str);
		wl_csa.reg = 0;
		wl_csa.frame_type = CSA_BROADCAST_ACTION_FRAME;

		WIFI_DBG("%s: send csa_broadcast %s chspec 0x%x beacon_count %d\n",
			__FUNCTION__, osifname, wl_csa.chspec, wl_csa.count);
		if (wl_iovar_set(osifname, "csa", &wl_csa, sizeof(wl_chan_switch_t)) != 0) {
			WIFI_ERR("%s: Error iovar_set csa\n", __FUNCTION__);
			return -1;
		}
	}
	return 0;
}

static int
dm_dcs_dwell_time(int cmd, int radioIndex, int *pvalue, uint *plen, char *pvar)
{
	char *nvifname, *osifname;
	char *nvram_value,  nvram_name[NVRAM_NAME_SIZE], dcsStr[STRING_LENGTH_32] = {0};
	int dcsDwellTime;

	if (strcmp(pvar, "dcs_dwell_time") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;
	}
	if (pvalue == NULL) {
		WIFI_ERR("%s: Null pvalue\n", __FUNCTION__);
		return -2;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);
	snprintf(nvram_name, sizeof(nvram_name), "%s_dcs_dwell_time", nvifname);
	if (cmd & CMD_GET_NVRAM)  {
		nvram_value = wlcsm_nvram_get(nvram_name);
		*pvalue = (!nvram_value) ? 0 : atoi(nvram_value);
		*plen = sizeof(*pvalue);
	}

	if (cmd & (CMD_SET_NVRAM)) {
		int wl_band, dt_min, dt_max;
		if (wl_get_wl_band(osifname, &wl_band) < 0) {
			WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
			return -1;
		}
		if (wl_band == WLC_BAND_6G) { /* 6G */
			dt_min = WIFIRADIO_DCSDWELLTIME_MIN_6G;
			dt_max = WIFIRADIO_DCSDWELLTIME_MAX_6G;
		} else {
			dt_min = WIFIRADIO_DCSDWELLTIME_MIN;
			dt_max = WIFIRADIO_DCSDWELLTIME_MAX;
		}
		dcsDwellTime = (int)(*pvalue);
		/* Adjust dwell_time as needed */
		if ((dcsDwellTime < dt_min) || (dcsDwellTime > dt_max)) {
			dcsDwellTime = (dt_min + dt_max) / 2;
		}
		snprintf(dcsStr, sizeof(dcsStr), "%d", dcsDwellTime);
		NVRAM_SET(nvram_name, dcsStr);
	}
	return 0;
}

static int
dm_acs_pref_chans(int cmd, int radioIndex, char *pvalue, uint *plen, char *pvar)
{
	char *nvifname = wldm_get_radio_nvifname(radioIndex);
	char *osifname = wldm_get_radio_osifname(radioIndex);
	char cmdBuf[WLDM_MAX_CH_LIST_LEN * STRING_LENGTH_16 + STRING_LENGTH_64] = {0};
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE];
	char out_chanspecs[WLDM_MAX_CH_LIST_LEN * STRING_LENGTH_16 + 1];
	uint chanspecs_len = sizeof(out_chanspecs), tmp_len = 0;

	if (strcmp(pvar, "acs_pref_chans") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;
	}
	if (pvalue == NULL) {
		WIFI_ERR("%s: Null pvalue\n", __FUNCTION__);
		return -2;
	}

	snprintf(nvram_name, sizeof(nvram_name), "%s_acs_excl_chans", nvifname);
	if (cmd & CMD_GET_NVRAM)  {
		/*  Get excluded chanspecs from nvram */
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value == NULL) {
			WIFI_ERR("%s: Null %s\n", __FUNCTION__, nvram_name);
			return -1;
		}
		tmp_len = *plen;
		/* Convert excluded chanspecs input from nvram to a list of included channels */
		if (wl_get_incl_channels(radioIndex, nvram_value, strlen(nvram_value),
			pvalue, plen) != 0) {
			WIFI_ERR("%s: Failed to get incl_channels, nvram_value=%s\n",
				__FUNCTION__, nvram_value);
			return -1;
		}
		WIFI_DBG("%s: pvalue=%s *plen=%d\n", __FUNCTION__, pvalue, *plen);
		if  (tmp_len < *plen) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
	}

	if (cmd & (CMD_SET_NVRAM | CMD_SET_IOCTL)) {
		if (*plen > (WL_NUMCHANNELS * CHSTRLEN - 1)) {
			WIFI_ERR("%s: buffer too short! *plen=%d\n", __FUNCTION__, *plen);
			return -1;
		}
		WIFI_DBG("%s: pvalue=%s *plen=%d\n", __FUNCTION__, pvalue, *plen);
		/* Convert channels or chanspecs input to list of excluded chanspecs */
		if (wl_get_excl_chanspecs(radioIndex, (char *)pvalue, strlen(pvalue),
			out_chanspecs, &chanspecs_len) != 0) {
			WIFI_ERR("%s: Failed to get excl_chanspecs, pvalue %s\n",
				__FUNCTION__, (char *)pvalue);
			return -1;
		}
		if (cmd & CMD_SET_IOCTL) {
			/*  Set excluded chanspecs thru acs_cli2 cmd */
			pid_t pid = get_pid_by_name(ACSD);
			if (pid <= 0) {
				WIFI_ERR("%s: acsd2 is not running\n", __FUNCTION__);
				return -1;
			}
			snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s set %s %s",
				ACS_CLI, osifname, "acs_excl_chans", out_chanspecs);
			if (system(cmdBuf) != 0) {
				WIFI_ERR("%s: system(%s) failed, errno:%d\n", __FUNCTION__, cmdBuf,
					errno);
				return -1;
			}
		}
		if (cmd & CMD_SET_NVRAM) {
			/*  Set excluded chanspecs to nvram */
			NVRAM_SET(nvram_name, out_chanspecs);
		}
	}
	return 0;
}

static xbrcm_t xbrcm_lq_tbl[] = {
	{  "bw_util",			{dm_chanstats},		CMD_GET,			},
	{  "channel_stats",		{dm_chanstats},		CMD_GET,			},
	{  "channel_stats2",		{dm_chanstats},		CMD_GET				},
	{  NULL,			{NULL},			0,				},
};

int
wldm_xbrcm_lq(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);
	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_lq_tbl);
}

/* Determine encryption/security mode of current AP entry
 */
static int
_wldm_get_security_mode(uint index, bool found_RSN, bool found_WPA, bool found_WPA2,
	bool found_WPA3, bool found_TKIP, bool found_AES, bool found_PSK, bool found_WEP,
	wldm_neighbor_ap2_t *neighbor)
{
	/* Get encryption mode of current entry */
	int ret = 0;
	uint i = index, securityModeEnabled = 0, encryptionMode = 0;
	if (found_RSN && found_WPA3 && found_WPA2) {
		securityModeEnabled = NEIGHBORDIAG_SECURITYMODEENABLED_WPA3_PERSONAL_TRANSITION;
		encryptionMode = NEIGHBORDIAG_ENCRYPTIONMODE_AES;
	} else if (found_RSN && found_WPA3) {
		securityModeEnabled = NEIGHBORDIAG_SECURITYMODEENABLED_WPA3_PERSONAL;
		encryptionMode = NEIGHBORDIAG_ENCRYPTIONMODE_AES;
	} else if (found_RSN && found_WPA) {
		securityModeEnabled = (found_PSK) ? NEIGHBORDIAG_SECURITYMODEENABLED_WPA_WPA2 :
			NEIGHBORDIAG_SECURITYMODEENABLED_WPA_WPA2_ENTERPRISE;
		encryptionMode = (found_TKIP && found_AES) ? NEIGHBORDIAG_ENCRYPTIONMODE_TKIPandAES :
			(found_TKIP) ? NEIGHBORDIAG_ENCRYPTIONMODE_TKIP : NEIGHBORDIAG_ENCRYPTIONMODE_AES;
	} else if (found_RSN && found_WPA2) {
		securityModeEnabled = (found_PSK) ? NEIGHBORDIAG_SECURITYMODEENABLED_WPA2:
			NEIGHBORDIAG_SECURITYMODEENABLED_WPA2_ENTERPRISE;
		encryptionMode = NEIGHBORDIAG_ENCRYPTIONMODE_AES;
	} else if (found_WPA) {
		WIFI_ERR("%s:%d Security mode: WPA invalid\n", __FUNCTION__, __LINE__);
	} else if (found_WEP) {
		securityModeEnabled = NEIGHBORDIAG_SECURITYMODEENABLED_WEP;
	} else {
		securityModeEnabled = NEIGHBORDIAG_SECURITYMODEENABLED_NONE;
	}
	if (securityModeEnabled >= 0 && securityModeEnabled <= NEIGHBOR_SECURITY_MODE_MAX) {
		strncpy(neighbor[i].ap_SecurityModeEnabled, neighborSecurityMode[securityModeEnabled],
			sizeof(neighbor[i].ap_SecurityModeEnabled));
	}
	if (encryptionMode >= 0 && encryptionMode <= NEIGHBOR_ENC_MODE_MAX) {
		strncpy(neighbor[i].ap_EncryptionMode, neighborEncryptionMode[encryptionMode],
			sizeof(neighbor[i].ap_EncryptionMode));
	}
	return ret;
}

/* Determine operating standars of current AP entry
 */
static int
_wldm_get_standards(uint index, bool found_HT, bool found_VHT, bool found_HE,
	wldm_neighbor_ap2_t *neighbor)
{
	/* Get operating standards of current entry */
	int ret = 0;
	uint i = index, channel = neighbor[i].ap_Channel, len;
	char pRates[STRING_LENGTH_64], *pch;
	bool support_b = FALSE, support_g = FALSE;

	strncpy(pRates, neighbor[i].ap_SupportedDataTransferRates, sizeof(*pRates));
	pch = strtok(pRates, ",");

	len = sizeof(neighbor[i].ap_SupportedStandards);
	memset(neighbor[i].ap_SupportedStandards, 0, len);
	memset(neighbor[i].ap_OperatingStandards, 0, sizeof(neighbor[i].ap_OperatingStandards));

	len -= 1; /* Reserve ending '\0' for strncat */
	if (channel >= WIFI_LOWEST_5G_CHANNEL_NUMBER) {
		strncat(neighbor[i].ap_SupportedStandards, "a,", len);
		len -= strlen("a,");
	} else if (channel <= WIFI_HIGHEST_2_4_G_CHANNEL_NUMBER) {
		while (pch != NULL) {
			if ((strcmp(pch, "1") == 0) || (strcmp(pch, "2") == 0) ||
				(strcmp(pch, "5.5") == 0) || (strcmp(pch, "11") == 0)) {
				support_b = TRUE;
			}
			if ((strcmp(pch, "6") == 0) || (strcmp(pch, "9") == 0) ||
				(strcmp(pch, "12") == 0) || (strcmp(pch, "18") == 0) ||
				(strcmp(pch, "24") == 0) || (strcmp(pch, "36") == 0) ||
				(strcmp(pch, "48") == 0) || (strcmp(pch, "54") == 0)) {
				support_g = TRUE;
			}
			pch = strtok(NULL, ",");
		}
	}
	if (support_b == TRUE) {
		strncat(neighbor[i].ap_SupportedStandards, "b,", len);
		len -= strlen("b,");
	}
	if (support_g == TRUE) {
		strncat(neighbor[i].ap_SupportedStandards, "g,", len);
		len -= strlen("g,");
	}
	if (found_HT) {
		strncat(neighbor[i].ap_SupportedStandards, "n,", len);
		len -= strlen("n,");
	} else if (found_VHT) {
		strncat(neighbor[i].ap_SupportedStandards, "ac,", len);
		len -= strlen("ac,");
	} else if (found_HE) {
		strncat(neighbor[i].ap_SupportedStandards, "ax,", len);
		len -= strlen("ax,");
	}

	/* Remove the last ',' */
	len = strlen(neighbor[i].ap_SupportedStandards);
	neighbor[i].ap_SupportedStandards[len - 1] = '\0';
	/* Set OperatingStandards same as SupportedStandards */
	strncpy(neighbor[i].ap_OperatingStandards, neighbor[i].ap_SupportedStandards,
			sizeof(neighbor[i].ap_OperatingStandards));

	return ret;
}

/* Prepare input options to "wl escanresults" command
 * Assumes radio_index is already validated.
 * Assumes scan_params[radio_index].chan_list is already validated.
 */
static int
escanresult_cmd_prep(int radio_index, wldm_scan_params_t escan_params, char *buf, int buf_size)
{
	int ret = 0, i, tot_len, wl_band;
	char localbuf[BUF_SIZE] = {0}, *osifname;

	osifname = wldm_get_radio_osifname(radio_index);

	memset(buf, 0, buf_size);
	snprintf(buf, buf_size, ESCANRESULTS_CMD, radio_index);
	tot_len = strlen(buf) + 1; /* Reserve ending '\0' for strncat */

	/* Scan type */
	switch (escan_params.scan_mode) {
		case WLDM_RADIO_SCAN_MODE_OFFCHAN:
			snprintf(localbuf, sizeof(localbuf), SCAN_TYPE_OPTION, SCAN_TYPE_OFFCHAN);
			strncat(buf, localbuf, buf_size - tot_len);
			tot_len += strlen(localbuf);
			break;
		case WLDM_RADIO_SCAN_MODE_SURVEY:
			snprintf(localbuf, sizeof(localbuf), SCAN_TYPE_OPTION, SCAN_TYPE_PASSIVE);
			strncat(buf, localbuf, buf_size - tot_len);
			tot_len += strlen(localbuf);
			break;
		default:
			break;
	}
	/* Dwell time */
	snprintf(localbuf, sizeof(localbuf), DWELL_TIME_ACTIVE_OPTION,
			 escan_params.dwell_time);
	strncat(buf, localbuf, buf_size - tot_len);
	tot_len += strlen(localbuf);
	snprintf(localbuf, sizeof(localbuf), DWELL_TIME_PASSIVE_OPTION,
			 escan_params.dwell_time);
	strncat(buf, localbuf, buf_size - tot_len);
	tot_len += strlen(localbuf);

	/* Channel list */
	if (escan_params.scan_mode == WLDM_RADIO_SCAN_MODE_ONCHAN) {
		channel_info_t ci;

		memset(&ci, 0, sizeof(ci));
		ret = wl_ioctl(osifname, WLC_GET_CHANNEL, &ci, sizeof(ci));
		if (ret < 0) {
			WIFI_ERR("ioctl WLC_GET_CHANNEL failed \n");
			return -1;
		}
		snprintf(localbuf, sizeof(localbuf), CHANNEL_LIST_OPTION_UI32, ci.target_channel);
		strncat(buf, localbuf, buf_size - tot_len);
		tot_len += strlen(localbuf);
	} else if (escan_params.num_channels) {
		char chan[STRING_LENGTH_32] = {0};
		char chanlist[STRING_LENGTH_128] = {0};
		int len = sizeof(chanlist) - 1; /* Reserve ending '\0' for strncat */

		if (wl_get_wl_band(osifname, &wl_band) < 0) {
			WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
			return -1;
		}
		for (i = 0; i < escan_params.num_channels; i++) {
			if (wl_band == WLC_BAND_6G) {
				snprintf(chan, sizeof(chan), "6g%u,", escan_params.chan_list[i]);
			}
			else {
				snprintf(chan, sizeof(chan), "%u,", escan_params.chan_list[i]);
			}
			strncat(chanlist, chan, len);
			len -= strlen(chan);
		}
		len = strlen(chanlist);
		chanlist[len - 1] = '\0';
		snprintf(localbuf, sizeof(localbuf), CHANNEL_LIST_OPTION, chanlist);
		strncat(buf, localbuf, buf_size - tot_len);
		tot_len += strlen(localbuf);
	}

	/* In commandline, write escanresults to file */
	snprintf(localbuf, sizeof(localbuf), "> /var/wl%d_escanresults 2>&1", radio_index);
	strncat(buf, localbuf, buf_size - tot_len);

	return ret;
}

/* Get scan params from file */
static int
_wldm_get_scan_params_from_file(int radio_index, wldm_scan_params_t *escan_params)
{
	boolean found_scanMode = FALSE, found_dwellTime = FALSE, found_numChan = FALSE,
		found_chanList = FALSE;
	int ret = 0, channel_count = 0;
	char buf[BUF_SIZE], buf2[STRING_LENGTH_32], *wl_keyword, *substring;
	FILE *fp_scanparams;

	/* Read escan_params from file */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "/tmp/wl%d_scan_params", radio_index);
	if ((fp_scanparams = fopen(buf, "r")) == NULL) {
		WIFI_ERR("fopen %s failed \n", buf);
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	while (!feof(fp_scanparams)) {
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp_scanparams) == NULL) {
			WIFI_ERR("%s: Couldn't read or no data in /tmp/wl%d_scan_params\n",
					__FUNCTION__, radio_index);
		}

		/* SCANPARAMS_SCANMODE */
		wl_keyword = wifi_scanParams_mapTable[SCANPARAMS_SCANMODE].wl_keyword;
		if ((substring = strstr(buf, wl_keyword)) != NULL) {
			sscanf(substring + strlen(wl_keyword), "%d", (int *)&escan_params->scan_mode);
			found_scanMode = TRUE;
			continue;
		}

		/* SCANPARAMS_DWELLTIME */
		wl_keyword = wifi_scanParams_mapTable[SCANPARAMS_DWELLTIME].wl_keyword;
		if ((substring = strstr(buf, wl_keyword)) != NULL) {
			sscanf(substring + strlen(wl_keyword), "%d", &escan_params->dwell_time);
			found_dwellTime = TRUE;
			continue;
		}

		/* SCANPARAMS_NUMBEROFCHANNELS */
		wl_keyword = wifi_scanParams_mapTable[SCANPARAMS_NUMBEROFCHANNELS].wl_keyword;
		if ((substring = strstr(buf, wl_keyword)) != NULL) {
			sscanf(substring + strlen(wl_keyword), "%u", &escan_params->num_channels);
			found_numChan = TRUE;
			continue;
		}

		/* SCANPARAMS_CHANNELLIST */
		wl_keyword = wifi_scanParams_mapTable[SCANPARAMS_CHANNELLIST].wl_keyword;
		if ((substring = strstr(buf, wl_keyword)) != NULL) {
			/* Assumes found_Channels == TRUE, so malloc(chan_list) with valid num_channels */
			if ((escan_params->chan_list =
				(uint *)malloc(escan_params->num_channels * sizeof(uint))) == NULL) {
				WIFI_ERR("escan_params[%d].chan_list malloc  failed\n",
					radio_index);
				escan_params->chan_list = NULL;
				if (fp_scanparams) {
					fclose(fp_scanparams);
				}
				return -1;
			}

			substring = buf + strlen(wl_keyword);
			memset(buf2, 0, sizeof(buf2));
			while ((sscanf(substring, "%s", buf2)) != -1) {
				if (!strcmp(buf2, "]")) {
					if (escan_params->num_channels != channel_count) {
						WIFI_ERR("escan_params[%d].num_channels=%d != channel_count=%d\n",
							radio_index, escan_params->num_channels,
							channel_count);
						free(escan_params->chan_list);
						escan_params->chan_list = NULL;
						if (fp_scanparams) {
							fclose(fp_scanparams);
						}
						return -1;
					}
					found_chanList = TRUE;
					break;
				}
				sscanf(buf2, "%u", &escan_params->chan_list[channel_count++]);
				substring += strlen(buf2) + 1;
				memset(buf2, 0, sizeof(buf2));
			}
			continue;
		}
	}

	if (!found_scanMode || !found_dwellTime || !found_numChan || !found_chanList) {
		WIFI_ERR("Missing scan param, found_scanMode=%d found_dwellTime=%d found_numChan=%d \
			found_chanList=%d\n", found_scanMode, found_dwellTime, found_numChan,
			found_chanList);
		if (escan_params[radio_index].chan_list) {
			free(escan_params[radio_index].chan_list);
			escan_params[radio_index].chan_list = NULL;
		}
		if (fp_scanparams) {
			fflush(fp_scanparams);
			fclose(fp_scanparams);
		}
		return -1;
	}
	if (fp_scanparams) {
		fflush(fp_scanparams);
		fclose(fp_scanparams);
	}

	return ret;
}

/* Check escanresults file for failures and return number of AP entries
 * Assumes radio_index is already validated.
 * Assumes scan_params[radio_index].chan_list is already validated.
 */
static int
_wldm_check_escanresults_file(int radio_index, unsigned int *num_entries)
{
	int ret = 0;
	char buf[BUF_SIZE];
	FILE *fp_scanresults;
	char *wl_keyword, *substring;
	unsigned int ap_entries = 0;
	unsigned int scan_rejected = 0, misc_error = 0, event_status = 0;

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "/var/wl%d_escanresults", radio_index);

	if ((fp_scanresults = fopen(buf, "r")) == NULL) {
		WIFI_ERR("fopen %s failed \n", buf);
		return -1;
	}

	while (!feof(fp_scanresults)) {
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp_scanresults) == NULL) {
			WIFI_ERR("%s: Couldn't open, or no data in /var/wl%d_escanresults\n",
					__FUNCTION__, radio_index);
		}

		if ((!strlen(buf)) || (buf[0] == '\n')) {
			continue;
		}

		wl_keyword = wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_SSID].wl_keyword;
		if (!strncmp(buf, wl_keyword, strlen(wl_keyword))) {
			ap_entries++;
		}

		wl_keyword = wifi_escanErrors_mapTable[ESCANFAILS_SCANREJECTED].wl_keyword;
		if (strstr(buf, wl_keyword) != NULL) {
			scan_rejected++;
			WIFI_ERR("scan_rejected=%d, in DFS channel\n", scan_rejected);
			ret = -1;
		}

		wl_keyword = wifi_escanErrors_mapTable[ESCANFAILS_MISCERROR].wl_keyword;
		if (strstr(buf, wl_keyword) != NULL) {
			misc_error++;
			WIFI_DBG("misc_error=%d \n", misc_error);
			ret = -1;
			wl_keyword = wifi_escanErrors_mapTable[ESCANFAILS_STATUS].wl_keyword;
			if ((substring = strstr(buf, wl_keyword)) != NULL) {
				sscanf(substring + strlen(wl_keyword), "%d", &event_status);
				WIFI_DBG("event_status=%d %s\n", event_status,
					event_status == WLC_E_STATUS_NEWSCAN ? "scan aborted" : "");
			}
		}
	}
	*num_entries = ap_entries;
	/* If ap_entries is zero, no AP's detected or scan is rejected */
	if (!ap_entries) {
		WIFI_DBG("%s No AP entries radio_index=%d\n", __FUNCTION__, radio_index);
	}
	if (fp_scanresults) {
		fclose(fp_scanresults);
	}
	return ret;
}

/* Call "wl escanresults" command and write scanresults to wlX_escanresults file
 * Assumes index is already validated.
 */
static int
_wldm_run_wl_escanresults(int index)
{
	int ret = 0;
	char buf[BUF_SIZE];
	wldm_scan_params_t escan_params;

	/* Get scan params from file to global escan_params, malloc(chan_list) */
	if (_wldm_get_scan_params_from_file(index, &escan_params) != 0) {
		WIFI_ERR("get scan params failed, index %d\n", index);
		ret = -1;
	} else {
		memset(buf, 0, sizeof(buf));
		/* Use values from wifi_startNeighborScan api as input options to
		* "wl escanresults" command and write results to file.
		*/
		if (!escanresult_cmd_prep(index, escan_params, buf, sizeof(buf))) {
			/* Check if escanresults is already in progress */
			if (system(buf) == -1) {
				WIFI_ERR("%s: Couldn't issue %s, errno is %d\n", __FUNCTION__, buf, errno);
			}
		}
	}
	/* Free global chan_list */
	if (escan_params.chan_list) {
		free(escan_params.chan_list);
	}
	return ret;
}

/* Move AP entries from wlX_escanresults file to structure
 */
static int
escanresults_get_aps(int index, wldm_neighbor_ap2_t *neighbor, uint *array_size)
{
	bool found_RSN = FALSE, found_WPA = FALSE, found_WPA2 = FALSE, found_WPA3 = FALSE,
		found_TKIP = FALSE, found_AES = FALSE, found_PSK = FALSE, found_WEP = FALSE,
		found_HT = FALSE, found_VHT = FALSE, found_HE = FALSE, is_basic_rate = FALSE,
		is_6g = FALSE;
	char *wl_keyword, *substring, *subchar, buf[BUF_SIZE], channel[STRING_LENGTH_32],
		rates[STRING_LENGTH_32], *substring6g;
	unsigned int len = 0, i = 0;
	int ret = 0, channelBandwidth = 0, lfd;
	FILE *fp_scanresults;

	/* Open escanresults file */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "/var/wl%d_escanresults", index);
	while (TRUE) {
		if ((lfd = wldm_get_escanlock(index)) > 0) {
			break;
		}
		WIFI_DBG("Err: %s:%d get lock failed, lfd=%d %s, index %d\n", __FUNCTION__,
			__LINE__, lfd, "escan in progress waiting", index);
		sleep(5);
	}
	if ((fp_scanresults = fopen(buf, "r")) == NULL) {
		WIFI_ERR("fopen %s failed\n", buf);
		wldm_free_escanlock(lfd);
		return -1;
	}

	/* Parse detailed information for each neighboring AP */
	while ((!feof(fp_scanresults)) && (i < *array_size)) {
		memset(buf, 0, sizeof(buf));
		if (fgets(buf, sizeof(buf), fp_scanresults) == NULL) {
			WIFI_ERR("%s: Coulnd't read or no data in /var/wl%d_escanresults\n",
					__FUNCTION__, index);
		}

		/* Found a new entry */
		if ((!strlen(buf)) || (buf[0] == '\n')) {
			_wldm_get_security_mode(i, found_RSN, found_WPA, found_WPA2, found_WPA3,
				found_TKIP, found_AES, found_PSK, found_WEP, neighbor);
			_wldm_get_standards(i, found_HT, found_VHT, found_HE, neighbor);

			i++;
			found_RSN = found_WPA = found_WPA2 = found_WPA3 = found_TKIP = found_AES =
			   found_PSK = found_WEP = found_HT = found_VHT = found_HE = FALSE;
			continue;
		}
		/* NEIGHBORDIAG_SSID */
		wl_keyword = wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_SSID].wl_keyword;
		if (!strncmp(buf, wl_keyword, strlen(wl_keyword))) {
			sscanf(buf + strlen(wl_keyword) + 1, "%s", (neighbor[i].ap_SSID));
			len = strlen(neighbor[i].ap_SSID);
			neighbor[i].ap_SSID[len - 1] = '\0';
		}
		/* NEIGHBORDIAG_BSSID */
		wl_keyword = wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_BSSID].wl_keyword;
		if (!strncmp(buf, wl_keyword, strlen(wl_keyword))) {
			sscanf(buf + strlen(wl_keyword), "%s", neighbor[i].ap_BSSID);
		}
		/* NEIGHBORDIAG_MODE */
		wl_keyword = wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_MODE].wl_keyword;
		if (!strncmp(buf, wl_keyword, strlen(wl_keyword))) {
			char mode[STRING_LENGTH_32];
			memset(mode, 0, sizeof(mode));

			sscanf(buf + strlen(wl_keyword), "%s", mode);
			if (!strncmp(mode, NEIGHBORDIAG_MODE_MANAGED, strlen(mode))) {
				strncpy(neighbor[i].ap_Mode, "Infrastructure", sizeof(neighbor[i].ap_Mode));
			} else {
				strncpy(neighbor[i].ap_Mode, "AdHoc", sizeof(neighbor[i].ap_Mode));
			}
		}
		/* NEIGHBORDIAG_CHANNEL */
		/* NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH */
		/* NEIGHBORDIAG_OPERATINGFREQUENCYBAND */
		wl_keyword = wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_CHANNEL].wl_keyword;
		if ((substring = strstr(buf, wl_keyword)) != NULL) {
			memset(channel, 0, sizeof(channel));

			substring6g = strstr(substring, "6g");
			if (substring6g) {
				sscanf(substring6g + strlen("6g"), "%s", channel);
				is_6g = TRUE;
			}
			else {
				sscanf(substring + strlen(wl_keyword), "%s", channel);
			}
			if ((subchar = strchr(channel, '/')) != NULL) {
				*subchar = '\0';
				subchar++;
				if (!strcmp(subchar, NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_160)) {
					channelBandwidth = NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_160MHZ;
				} else if (!strcmp(subchar,
					NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_80)) {
					channelBandwidth = NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_80MHZ;
				} else {
					WIFI_ERR("Channel bandwidth invalid i=%d bw=%s \n",
							 i, subchar + 1);
				}
			} else if ((strstr(channel, NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_U)) ||
					(strstr(channel, NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_L))) {
				channelBandwidth = NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_40MHZ;
			} else {
				channelBandwidth = NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_20MHZ;
			}

			sscanf(channel, "%u", &neighbor[i].ap_Channel);
			if (neighbor[i].ap_Channel < WIFI_LOWEST_5G_CHANNEL_NUMBER) {
				strncpy(neighbor[i].ap_OperatingFrequencyBand, "2.4GHz",
						sizeof(neighbor[i].ap_OperatingFrequencyBand));
			} else {
				strncpy(neighbor[i].ap_OperatingFrequencyBand, "5GHz",
						 sizeof(neighbor[i].ap_OperatingFrequencyBand));
			}
			if (is_6g) {
				strncpy(neighbor[i].ap_OperatingFrequencyBand, "6GHz",
						 sizeof(neighbor[i].ap_OperatingFrequencyBand));
			}
			if (channelBandwidth >= NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_20MHZ &&
				channelBandwidth < NEIGHBORDIAG_OPERATINGCHANNELBANDWIDTH_MAX) {
				strncpy(neighbor[i].ap_OperatingChannelBandwidth,
						WLDM_OPERATINGBANDWIDTH[channelBandwidth],
						sizeof(neighbor[i].ap_OperatingChannelBandwidth));
			}
		}
		/* NEIGHBORDIAG_SIGNALSTRENGTH */
		wl_keyword =
			wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_SIGNALSTRENGTH].wl_keyword;
		if ((substring = strstr(buf, wl_keyword)) != NULL) {
			sscanf(substring + strlen(wl_keyword), "%d", &neighbor[i].ap_SignalStrength);
		}
		/* NEIGHBORDIAG_SECURITYMODEENABLED */
		/* NEIGHBORDIAG_ENCRYPTIONMODE */
		if (!strncmp(buf, NEIGHBORDIAG_RSN, strlen(NEIGHBORDIAG_RSN))) {
			found_RSN = TRUE;
		}
		if (strstr(buf, NEIGHBORDIAG_WPA3)) {
			found_WPA3 = TRUE;
		}
		if (strstr(buf, NEIGHBORDIAG_WPA2)) {
			found_WPA2 = TRUE;
		} else if (strstr(buf, NEIGHBORDIAG_WPA)) {
			found_WPA = TRUE;
		} else if ((substring = strstr(buf, NEIGHBORDIAG_CAPABILITY)) != NULL) {
			if (strstr(substring + strlen(NEIGHBORDIAG_CAPABILITY),
				NEIGHBORDIAG_WEP) != NULL) {
					found_WEP = TRUE;
			}
		} else if ((substring = strstr(buf, NEIGHBORDIAG_UNICAST_CIPHERS)) != NULL) {
			if (strstr(substring + strlen(NEIGHBORDIAG_UNICAST_CIPHERS),
				NEIGHBORDIAG_TKIP) != NULL) {
					found_TKIP = TRUE;
			}
			if (strstr(substring + strlen(NEIGHBORDIAG_UNICAST_CIPHERS),
				NEIGHBORDIAG_AES) != NULL) {
				found_AES = TRUE;
			}
		} else if ((substring = strstr(buf, NEIGHBORDIAG_AKM_SUITES)) != NULL) {
			if ((strstr(substring + strlen(NEIGHBORDIAG_AKM_SUITES),
				NEIGHBORDIAG_WPA_PSK) != NULL) ||
				(strstr(substring + strlen(NEIGHBORDIAG_AKM_SUITES),
				NEIGHBORDIAG_WPA2_PSK) != NULL)) {
				found_PSK = TRUE;
			}
		}

		/* NEIGHBORDIAG_SUPPORTEDSTANDARDS */
		/* NEIGHBORDIAG_OPERATINGSTANDARDS */
		if (!strncmp(buf, NEIGHBORDIAG_HT_CAPABLE, strlen(NEIGHBORDIAG_HT_CAPABLE))) {
			found_HT = TRUE;
		} else if (!strncmp(buf, NEIGHBORDIAG_VHT_CAPABLE,
				strlen(NEIGHBORDIAG_VHT_CAPABLE))) {
			found_VHT = TRUE;
		} else if (!strncmp(buf, NEIGHBORDIAG_HE_CAPABLE,
				strlen(NEIGHBORDIAG_HE_CAPABLE))) {
			found_HE = TRUE;
		}

		/* NEIGHBORDIAG_BEACONPERIOD */
		neighbor[i].ap_BeaconPeriod = NEIGHBORDIAG_BEACONPERIOD_DEFAULTS;
		/* NEIGHBORDIAG_NOISE */
		wl_keyword = wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_NOISE].wl_keyword;
		if ((substring = strstr(buf, wl_keyword)) != NULL) {
			sscanf(substring + strlen(wl_keyword), "%d", &neighbor[i].ap_Noise);
		}
		/* NEIGHBORDIAG_SUPPORTEDRATES */
		/* NEIGHBORDIAG_BASICRATES */
		memset(rates, 0, sizeof(rates));

		wl_keyword =
		   wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_SUPPORTEDRATES].wl_keyword;
		if (!strncmp(buf, wl_keyword, strlen(wl_keyword))) {
			memset(neighbor[i].ap_SupportedDataTransferRates, 0,
				   sizeof(neighbor[i].ap_SupportedDataTransferRates));
			memset(neighbor[i].ap_BasicDataTransferRates, 0,
				   sizeof(neighbor[i].ap_BasicDataTransferRates));
			substring = buf + strlen(wl_keyword) + 2;
			while ((sscanf(substring, "%s", rates)) != -1) {
				if (!strcmp(rates, "]")) break;
				substring += strlen(rates) + 1;
				if ((subchar = strchr(rates, '(')) != NULL) {
					*subchar = '\0';
					is_basic_rate = TRUE;
				} else {
					is_basic_rate = FALSE;
				}
				len = sizeof(neighbor[i].ap_SupportedDataTransferRates) -
					strlen(neighbor[i].ap_SupportedDataTransferRates) - 1;
				strncat(neighbor[i].ap_SupportedDataTransferRates, rates, len);
				len -= strlen(rates);
				strncat(neighbor[i].ap_SupportedDataTransferRates, ",", len);
				if (is_basic_rate) {
					len = sizeof(neighbor[i].ap_BasicDataTransferRates) -
						strlen(neighbor[i].ap_BasicDataTransferRates) - 1;
					strncat(neighbor[i].ap_BasicDataTransferRates, rates, len);
					len -= strlen(rates);
					strncat(neighbor[i].ap_BasicDataTransferRates, ",", len);
				}
				memset(rates, 0, sizeof(rates));
			}
			/* Remove the last ',' */
			len = strlen(neighbor[i].ap_SupportedDataTransferRates);
			neighbor[i].ap_SupportedDataTransferRates[len - 1] = '\0';
			len = strlen(neighbor[i].ap_BasicDataTransferRates);
			neighbor[i].ap_BasicDataTransferRates[len - 1] = '\0';
		}
		/* NEIGHBORDIAG_DTIMPERIOD */
		neighbor[i].ap_DTIMPeriod = NEIGHBORDIAG_DTIMPERIOD_DEFAULTS;

		/* NEIGHBORDIAG_QBSSUTILIZATION */
		wl_keyword =
		   wifi_neighborDiagsRes_mapTable[NEIGHBORDIAG_QBSSUTILIZATION].wl_keyword;
		if (!strncmp(buf, wl_keyword, strlen(wl_keyword))) {
			substring = strstr(buf, "(");
			sscanf(substring + 1, "%d", &neighbor[i].ap_ChannelUtilization);
		}
	}

	fclose(fp_scanresults);
	wldm_free_escanlock(lfd);

	*array_size = i;
	return ret;
}

static int
dm_scan(int cmd, int index, wldm_scan_params_t *wldm_scan_params, uint *plen, char *pvar)
{
	int ret = 0, len, dtime = wldm_scan_params->dwell_time;
	FILE *fp_scanparams;
	char buf[STRING_LENGTH_128], buf2[STRING_LENGTH_32];
	uint i = 0;
	int lfd = -1;

	if (cmd & CMD_SET_IOCTL) {
		snprintf(buf, sizeof(buf), "/tmp/wl%d_scan_params", index);
		if ((lfd = wldm_get_escanlock(index)) < 0) {
			WIFI_ERR("Err: %s:%d get lock failed, lfd=%d %s, index %d\n", __FUNCTION__,
				__LINE__, lfd,"escan already in progress", index);
			return -1;
		}
		if (!(fp_scanparams = fopen(buf, "w"))) {
			WIFI_ERR("Err: %s:%d fopen %s failed\n", __FUNCTION__, __LINE__, buf);
			wldm_free_escanlock(lfd);
			return -1;
		}
		if (wldm_scan_params->scan_mode == WLDM_RADIO_SCAN_MODE_DCS) {
			char *nvifname, *nvram_value, nvram_name[NVRAM_NAME_SIZE];

			nvifname = wldm_get_radio_nvifname(index);
			snprintf(nvram_name, sizeof(nvram_name), "%s_dcs_dwell_time", nvifname);
			nvram_value = wlcsm_nvram_get(nvram_name);
			wldm_scan_params->dwell_time = (!nvram_value) ? 0 : atoi(nvram_value);
			wldm_scan_params->scan_mode = WLDM_RADIO_SCAN_MODE_NONE;
		}

		fprintf(fp_scanparams, "%s%d\n",
			wifi_scanParams_mapTable[SCANPARAMS_SCANMODE].wl_keyword,
			wldm_scan_params->scan_mode);
		if (index == 2) { /* is 6G */
			BOUNDS_SET(dtime, WIFIRADIO_DCSDWELLTIME_MIN_6G,
				WIFIRADIO_DCSDWELLTIME_MAX_6G);
		}
		else {
			BOUNDS_SET(dtime, WIFIRADIO_DCSDWELLTIME_MIN,
				WIFIRADIO_DCSDWELLTIME_MAX);
		}
		fprintf(fp_scanparams, "%s%d\n",
			wifi_scanParams_mapTable[SCANPARAMS_DWELLTIME].wl_keyword, dtime);
		fprintf(fp_scanparams, "%s%d\n",
			wifi_scanParams_mapTable[SCANPARAMS_NUMBEROFCHANNELS].wl_keyword,
			wldm_scan_params->num_channels);

		snprintf(buf, sizeof(buf), "%s",
			wifi_scanParams_mapTable[SCANPARAMS_CHANNELLIST].wl_keyword);
		len = sizeof(buf) - strlen(buf) - 1; /* Reserve ending '\0' for strncat */
		for (i = 0; i < wldm_scan_params->num_channels; i++) {
			snprintf(buf2, sizeof(buf2), "%d ", wldm_scan_params->chan_list[i]);
			strncat(buf, buf2, len);
			len -= strlen(buf2);
		}
		/* Indicate end of list with "]" */
		fprintf(fp_scanparams, "%s]\n", buf);
		fflush(fp_scanparams);
		fclose(fp_scanparams);

		ret = _wldm_run_wl_escanresults(index);
		wldm_free_escanlock(lfd);
	}
	return ret;
}

static int
dm_scanresults(int cmd, int index, void *neighbor_ap_array, uint *plen, char *pvar)
{
	int ret = 0;
	if (cmd & CMD_GET) {
		if (escanresults_get_aps(index, (wldm_neighbor_ap2_t *)neighbor_ap_array, plen)) {
			WIFI_ERR("Err: %s:%d escanresults_get_aps FAIL index %d\n",
				__FUNCTION__, __LINE__, index);
			ret = -1;
		}
	}
	return ret;
}

static int
dm_num_scanresults(int cmd, int index, void *pvalue, uint *plen, char *pvar)
{
	int ret = 0, lfd;
	if (cmd & CMD_GET) {
		while (TRUE) {
			if ((lfd = wldm_get_escanlock(index)) > 0) {
				break;
			}
			WIFI_DBG("Err: %s:%d get lock failed, lfd=%d %s, index %d\n",
				__FUNCTION__, __LINE__, lfd, "escan in progress waiting", index);
			sleep(5);
		}
		*plen = 0;
		if (_wldm_check_escanresults_file(index, plen) != 0) {
			WIFI_ERR("Err: %s:%d _wldm_check_escanresults_file index %d\n",
				__FUNCTION__, __LINE__, index);
			ret = -1;
		}
		wldm_free_escanlock(lfd);
	}
	return ret;
}

static xbrcm_t xbrcm_scan_tbl[] = {
	{  "scan",			{dm_scan},		CMD_SET_IOCTL,			},
	{  "scan_results",		{dm_scanresults},	CMD_GET,			},
	{  "num_scan_results",		{dm_num_scanresults},	CMD_GET,			},
	{  "dcs_dwell_time",		{dm_dcs_dwell_time},	CMD_GET_NVRAM | CMD_SET_NVRAM,	},
	{  "acs_pref_chans",		{dm_acs_pref_chans},	CMD_GET_NVRAM | CMD_SET_NVRAM | CMD_SET_IOCTL,	},
	{  NULL,			{NULL},			0,				},
};

int
wldm_xbrcm_scan(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);
	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_scan_tbl);
}

int
wldm_Radio_ChannelsInUse(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ChannelsInUse";
	char chspec_str[WL_NUMCHANNELS*5 + 1] = {0}; /* allow 3digits, space, comma */
	wldm_scan_params_t wldm_scan_params;
	wldm_neighbor_ap2_t *wldm_neighbor_ptr = NULL;
	unsigned int len, num = 0, clen, i;
	char apChStr[16];
	char *osifname;
	channel_info_t mchanInfo;
	unsigned int mchannel;

	osifname = wldm_get_radio_osifname(radioIndex);

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_GET_NVRAM |
		CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		int wl_band;

		/* Start Scan */
		memset(&wldm_scan_params, 0, sizeof(wldm_scan_params));
		wldm_scan_params.scan_mode = WIFI_RADIO_SCAN_MODE_FULL;
		if (wl_get_wl_band(osifname, &wl_band) < 0) {
			WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
			return -1;
		}
		if (wl_band == WLC_BAND_6G) { /* 6G uses received beacons on PSC channels */
			wldm_scan_params.dwell_time = 110;
		} else {
			wldm_scan_params.dwell_time = 20;
		}
		wldm_scan_params.chan_list = NULL;
		WIFI_DBG("%s: Start Scan radioIndex [%d] scan_mode=%d dwell_time=%d\n",
			__FUNCTION__, radioIndex, wldm_scan_params.scan_mode,
			wldm_scan_params.dwell_time);
		if (wldm_xbrcm_scan(CMD_SET_IOCTL, radioIndex, (void *)&wldm_scan_params, &len,
			"scan", NULL) != 0) {
			WIFI_ERR("%s: Failed Scan radioIndex [%d] \n", __FUNCTION__, radioIndex);
			return -1;
		}

		/* Scan Results */
		if (wldm_xbrcm_scan(CMD_GET, radioIndex, NULL, &num, "num_scan_results",
			NULL) < 0) {
			WIFI_ERR("%s: radio %d wldm_xbrcm_scan failed\n", __FUNCTION__, radioIndex);
			return -2;
		}

		WIFI_DBG("%s: num scan results = %d radioIndex [%d]\n", __FUNCTION__, num,
			radioIndex);

		if (num > 0) {
			len = num * sizeof(wldm_neighbor_ap2_t);
			wldm_neighbor_ptr = (wldm_neighbor_ap2_t *)malloc(len);
			if (wldm_neighbor_ptr == NULL) {
				WIFI_ERR("%s: radio %d Malloc failed\n", __FUNCTION__,  radioIndex);
				return -3;
			}
			memset(wldm_neighbor_ptr, 0, len);
			if (wldm_xbrcm_scan(CMD_GET, radioIndex, (void *)wldm_neighbor_ptr, &num,
				"scan_results", NULL) < 0) {
				WIFI_ERR("%s: radio %d wldm_xbrcm_scan failed\n",
					__FUNCTION__, radioIndex);
				free(wldm_neighbor_ptr);
				return -4;
			}
		} /* num > 0 */

		/* Include the current channel in the list */
		if (wl_ioctl(osifname, WLC_GET_CHANNEL, &mchanInfo, sizeof(mchanInfo)) < 0) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_CHANNEL failed!\n", __FUNCTION__);
			return -1;
		}
		mchannel = mchanInfo.target_channel;
		snprintf(chspec_str, sizeof(chspec_str), " %d,", mchannel);

		/* Add channels from scanresults to the list */
		clen = strlen(chspec_str);
		for (i = 0; i < num; ++i) {
			snprintf(apChStr, sizeof(apChStr), " %d,", wldm_neighbor_ptr[i].ap_Channel);
			if (strstr(chspec_str, apChStr)) {
				/* already there */
				continue;
			}
			clen += strlen(apChStr);
			if (clen >= sizeof(chspec_str)) {
				WIFI_ERR("%s: radio %d chspec_str too long %d truncating\n",
					__FUNCTION__, radioIndex, clen);
				break;
				/* not returning error */
			}
			strcat(chspec_str, apChStr);
		}
		clen = strlen(chspec_str);
		chspec_str[clen - 1] = '\0'; /* last , to null */
		WIFI_DBG("%s: chspec_str = %s radioIndex [%d]\n", __FUNCTION__, chspec_str,
			radioIndex);
		remSpaceInStr(chspec_str, clen, chspec_str);
		if (*plen < strlen(chspec_str)) {
			WIFI_ERR("%s: radio %d buffer size %d too short\n", __FUNCTION__,
				radioIndex, *plen);
			if (wldm_neighbor_ptr) {
				free(wldm_neighbor_ptr);
			}
			return -5;
		}
		strcpy(pvalue, chspec_str);
		*plen = strlen(chspec_str);

		if (wldm_neighbor_ptr) {
			free(wldm_neighbor_ptr);
		}

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}
	return 0;
}

static char *
strntolower(char *str, size_t n)
{
	char *s = str;
	int i = 0;

	while (*s && i < n) {
		*s = tolower(*s);
		s++;
		i++;
	}

	*s = '\0';
	return str;
}

#define TR181_CNTRY_SZ		2
#define TR181_ENV_SUFFIX_SZ	1
#define TR181_REG_DOMAIN_SZ	(TR181_CNTRY_SZ + TR181_ENV_SUFFIX_SZ)
static bool
isValidCountryName(char *country_abbrev)
{
	unsigned int count = 0;
	int array_size = sizeof(cntry_names)/sizeof(cntry_names[0]);

	for (count = 0; count < (array_size - 1); count++) {
		if (!strncmp(country_abbrev, cntry_names[count].abbrev, TR181_CNTRY_SZ)) {
			return TRUE;
		}
	}

	return FALSE;
}

int
wldm_Radio_RegulatoryDomain(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RegulatoryDomain";
	char *nvifname, *osifname;
	char buf[WLC_CNTRY_BUF_SZ + 1], nvram_name[NVRAM_NAME_SIZE], *nvram_value, nv_buf[BUF_SIZE] = {0};
	wl_country_t country_spec = {{0}, 0, {0}};
	char *token, *saveptr, country_abbrev[WLC_CNTRY_BUF_SZ] = {0};
	char suffix[TR181_ENV_SUFFIX_SZ + 1] = {'I', '\0'};
	int len;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (*plen < sizeof(buf)) {
			WIFI_ERR("%s: *plen %d is less than %d\n", __FUNCTION__, *plen, sizeof(buf));
			return -2;
		}

		if (wl_iovar_get(osifname, "country", &country_spec, sizeof(country_spec)) < 0) {
			WIFI_ERR("%s: wl_iovar_get() country failed!\n", __FUNCTION__);
			return -1;
		}

		snprintf(buf, sizeof(buf), "%s%s", country_spec.country_abbrev, suffix);

		snprintf(pvalue, *plen, "%s", buf);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	if (cmd & (CMD_SET | CMD_SET_NVRAM | CMD_SET_IOCTL)) {
		len = strlen(pvalue);
		if (len > TR181_REG_DOMAIN_SZ || len < TR181_CNTRY_SZ) {
			WIFI_ERR("%s: invalid pvalue len %d, expected no less than %d\n", __FUNCTION__,
				len, TR181_CNTRY_SZ);
			return -1;
		}

		suffix[0] = pvalue[TR181_CNTRY_SZ];
		if (suffix[0] == '\0') {
			suffix[0] = ' ';
		} else if (suffix[0] != 'I' && suffix[0] != 'O' && suffix[0] != ' ') {
			WIFI_ERR("%s: invalid regulatory domain suffix %s\n", __FUNCTION__, suffix);
			return -2;
		}

		strncpy(country_abbrev, pvalue, TR181_CNTRY_SZ);
		if (!isValidCountryName(country_abbrev)) {
			WIFI_ERR("%s: invalid country_abbrev %s\n", __FUNCTION__, country_abbrev);
			return -4;
		}

		WIFI_DBG("%s: country_abbrev = %s env suffix = %s\n", __FUNCTION__, country_abbrev, suffix);

		strncpy(country_spec.country_abbrev, country_abbrev, sizeof(country_spec.country_abbrev));
		snprintf(nvram_name, sizeof(nvram_name), "%s_country_%s", nvifname,
			strntolower(country_abbrev, sizeof(country_abbrev)));
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (!nvram_value) {
			WIFI_ERR("%s: missing nvram %s for country domain %s\n", __FUNCTION__, nvram_name,
				   country_spec.country_abbrev);
			return -5;
		}

		strncpy(nv_buf, nvram_value, sizeof(nv_buf));
		token = strtok_r(nv_buf, "/", &saveptr);
		if (token && (strlen(token) == TR181_CNTRY_SZ)) {
			memcpy(country_spec.ccode, token, sizeof(country_spec.ccode));
			WIFI_DBG("%s: ccode = %s\n", __FUNCTION__, country_spec.ccode);
		} else {
			WIFI_ERR("%s: invalid ccode/regrev pair %s or ccode length\n", __FUNCTION__, nvram_value);
			return -6;
		}

		token = strtok_r(NULL, "/", &saveptr);
		if (token)
			country_spec.rev = atoi(token);
		else
			country_spec.rev = -1;

		if (cmd & CMD_SET) {
			Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_RegulatoryDomain_MASK);
			if (pObj == NULL)
				return -7;

			pObj->Radio.RegulatoryDomain = (char *) malloc(len + 1);
			if (pObj->Radio.RegulatoryDomain == NULL) {
				WIFI_ERR("%s: malloc failure\n", __FUNCTION__);
				wldm_rel_Object(pObj, FALSE);
				return -8;
			}
			strncpy(pObj->Radio.RegulatoryDomain, pvalue, len + 1);
			pObj->apply_map |= Radio_RegulatoryDomain_MASK;
			wldm_rel_Object(pObj, TRUE);
		}

		if (cmd & CMD_SET_NVRAM) {
			snprintf(nvram_name, sizeof(nvram_name), "%s_country_code", nvifname);
			snprintf(buf, sizeof(buf), "%s", country_spec.ccode);
			if (wlcsm_nvram_set(nvram_name, buf) != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
				return -9;
			}

			snprintf(nvram_name, sizeof(nvram_name), "%s_country_rev", nvifname);
			snprintf(buf, sizeof(buf), "%d", country_spec.rev);
			if (wlcsm_nvram_set(nvram_name, buf) != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
				return -10;
			}
		}

		if (cmd & CMD_SET_IOCTL) {
			if (wl_iovar_set(osifname, "country", &country_spec, sizeof(country_spec))) {
				WIFI_ERR("%s: wl_iovar_set() country failed!\n", __FUNCTION__);
				return -11;
			}
			WIFI_DBG("%s: wl_ioctl for country %s/%d is done !\n", __FUNCTION__, country_spec.country_abbrev,
					country_spec.rev);
		}
	}

	return 0;
}

int
wldm_Radio_OperatingFrequencyBand(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "OperatingFrequencyBand";
	char *osifname;
	int band;

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_BAND failed!\n", __FUNCTION__);
			return -1;
		}
		band = dtoh32(band);

		if (band == WLC_BAND_AUTO) {
			/* convert to specific operating band */
			chanspec_t chanspec;

			if (wl_iovar_getint(osifname, "chanspec", (int*)&chanspec) < 0) {
				WIFI_ERR("%s: wl_iovar_getint chanspec failed\n", __FUNCTION__);
				return -1;
			}
			band = CHSPEC_IS2G(chanspec) ? WLC_BAND_2G :
				(CHSPEC_IS5G(chanspec) ? WLC_BAND_5G : WLC_BAND_6G);
		}

		if (band == WLC_BAND_5G) {
			strcpy(pvalue, "a");
		} else if (band == WLC_BAND_2G) {
			strcpy(pvalue, "b");
		} else if (band == WLC_BAND_6G) {
			strcpy(pvalue, "6g");
		} else {
			WIFI_ERR("%s : unrecognized band value %d wl_ioctl WLC_GET_BAND\n",
				__FUNCTION__, band);
			return -1;
		}

		if (*plen < strlen(pvalue))
			return -1;

		*plen = strlen(pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	return 0;
}

/* Returns number of valid channels or -1 if IOCTL failure or *plen < possible channel string len */
int
wldm_Radio_PossibleChannels(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "PossibleChannels";
	char *osifname;
	wl_uint32_list_t *list = NULL;
	uint32 possibleChannels[WL_NUMCHANNELS + 1] = {0};
	int idx = 0;
	int count = 0;
	char possibleChannelsStr[BUF_SIZE] = {0};
	int loc = 0;

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		list = (wl_uint32_list_t *)possibleChannels;
		list->count = htod32(WL_NUMCHANNELS);
		if (wl_ioctl(osifname, WLC_GET_VALID_CHANNELS, possibleChannels,
				sizeof(possibleChannels)) < 0) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_VALID_CHANNELS failed!\n", __FUNCTION__);
			return -1;
		}
		list->count = dtoh32(list->count);

		for (idx = 0; idx < list->count; idx++) {
			count = sprintf(&possibleChannelsStr[loc], "%d,", list->element[idx]);
			loc += count;
		}
		possibleChannelsStr[loc-1] = '\0';

		if (*plen < strlen(possibleChannelsStr))
			return -1;

		strcpy(pvalue, possibleChannelsStr);
		*plen = strlen(pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	return list->count;
}

int
wldm_Radio_Status(int cmd, int radioIndex, boolean *pvalue,
	int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Status";
	char *osifname;
	int  Radio_Status;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_ioctl(osifname, WLC_GET_UP, &Radio_Status, *plen) < 0) {
			WIFI_ERR("%s: wl_ioctl WLC_GET_UP failed!\n", __FUNCTION__);
			return -1;
		}
		*pvalue = Radio_Status;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	return 0;
}

int
wldm_Radio_RxChainMask(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RxChainMask";
	char *osifname;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if ((wl_iovar_getint(osifname, "rxchain", pvalue)) < 0) {
			WIFI_ERR("%s: wl_iovar_getint rxchain failed!\n", __FUNCTION__);
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_Radio_BeaconPeriod(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "BeaconPeriod";
	char *nvifname, *osifname;
	char  buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_ioctl(osifname, WLC_GET_BCNPRD, pvalue, sizeof(*pvalue)) < 0) {
			WIFI_ERR("%s: wl_ioctl WLC_GET_BCNPRD failed!\n", __FUNCTION__);
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_BeaconPeriod_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Radio.BeaconPeriod = *pvalue;
		pObj->apply_map |= Radio_BeaconPeriod_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_bcn", nvifname);
		snprintf(buf, sizeof(buf), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET) {
		if (wl_ioctl(osifname, WLC_SET_BCNPRD, pvalue, *plen) < 0) {
			WIFI_ERR("%s: wl_ioctl WLC_SET_BCNPRD failed!\n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: wl_ioctl done !\n", __FUNCTION__);
	}

	return 0;
}

int
wldm_Radio_DTIMPeriod(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "DTIMPeriod";
	char *nvifname, *osifname;
	int err;
	char buf[BUF_SIZE] = {'\0'};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);
	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}
	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		err = wl_ioctl(osifname, WLC_GET_DTIMPRD, pvalue, *plen);
		if (err < 0) {
			WIFI_ERR("%s: wl_ioctl  WLC_GET_DTIMPRD failed!\n", __FUNCTION__);
			return -1;
		}
		if (cmd & CMD_LIST) {
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
			return 0;
		}
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_DTIMPeriod_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Radio.DTIMPeriod = *pvalue;
		pObj->apply_map |= Radio_DTIMPeriod_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_dtim", nvifname);
		snprintf(buf, sizeof(buf), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		err = wl_ioctl(osifname, WLC_SET_DTIMPRD, pvalue, *plen);
		if (err < 0) {
			WIFI_ERR("%s: wl_ioctl  WLC_SET_DTIMPRD failed!\n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: wl_ioctl done !\n", __FUNCTION__);
	}

	return 0;
}

int
wldm_Radio_MaxBitRate(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "MaxBitRate";
	char *osifname;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_ioctl(osifname, WLC_GET_RATE, pvalue, *plen) < 0) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_RATE failed!\n", __FUNCTION__);
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_Radio_AMSDUEnable(int cmd, int radioIndex, boolean *pvalue,
	int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "amsdu";
	char *osifname;
	int err;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "amsdu", (int *) pvalue) < 0) {
			WIFI_ERR("%s: wl_ioctl WLC_GET_UP failed!\n", __FUNCTION__);
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}
	if (cmd & CMD_SET) {
		X_RDK_Radio_Object *pObj = wldm_get_X_RDK_RadioObject(radioIndex,
						X_RDK_Radio_AmsduEnable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->X_RDK_Radio.amsdu = *pvalue;
		pObj->apply_map |= X_RDK_Radio_AmsduEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_IOCTL) {
		err = wl_iovar_setint(osifname, "amsdu", *pvalue);
		if (err < 0) {
			WIFI_ERR("%s: wl_ioctl  WLC_SET_DTIMPRD failed!\n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: wl_ioctl done !\n", __FUNCTION__);
	}

	return 0;
}

int
wldm_Radio_IEEE80211hEnabled(int cmd, int radioIndex, boolean *pvalue,
	int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "IEEE80211hEnabled";
	char *nvifname, *osifname;
	int ieee80211Enabled;
	char buf[BUF_SIZE] = {'\0'};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_ioctl(osifname, WLC_GET_SPECT_MANAGMENT, &ieee80211Enabled,
				sizeof(ieee80211Enabled)) < 0) {
			WIFI_ERR("%s: wl_ioctl WLC_GET_SPECT_MANAGMENT failed!\n", __FUNCTION__);
			return -1;
		}

		(ieee80211Enabled == 0) ? (*pvalue = FALSE) : (*pvalue = TRUE);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_IEEE80211hEnabled_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Radio.IEEE80211hEnabled = *pvalue;
		pObj->apply_map |=  Radio_IEEE80211hEnabled_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_reg_mode", nvifname);

		if (*pvalue)
			buf[0] = 'h';
		else
			snprintf(buf, sizeof(buf), "%s", "off");

		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
					__FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		(*pvalue == 0) ? (ieee80211Enabled = 0) : (ieee80211Enabled = 1);

		if (wl_ioctl(osifname, WLC_SET_SPECT_MANAGMENT, &ieee80211Enabled,
				sizeof(ieee80211Enabled)) < 0) {
			WIFI_ERR("%s: wl_ioctl WLC_SET_SPECT_MANAGMENT failed!\n", __FUNCTION__);
			return -1;
		}
	}

	return 0;
}

int
wldm_Radio_AutoChannelEnable(int cmd, int radioIndex, boolean *pvalue,
    int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AutoChannelEnable";
	char *nvifname, *osifname;
	char cmdBuf[BUF_SIZE] = {0};
	char buf[BUF_SIZE] = {0};
	char nvram_name[NVRAM_NAME_SIZE] = {0}, *nvram_value;
	int pid, val;
	bool acs_boot_only = FALSE;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_chanspec", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value != NULL) {
			val = atoi(nvram_value);
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get %s failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}

		*pvalue = (val == 0) ? TRUE: FALSE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST) {
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "enabled" : "disabled");
		}
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_AutoChannelEnable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Radio.AutoChannelEnable = *pvalue;
		pObj->apply_map |= Radio_AutoChannelEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		if (*pvalue) {
			NVRAM_SET("acsd_restart", "yes");
			snprintf(nvram_name, sizeof(nvram_name), "%s_channel", nvifname);
			NVRAM_SET(nvram_name, "0");
			snprintf(nvram_name, sizeof(nvram_name), "%s_chanspec", nvifname);
			NVRAM_SET(nvram_name, "0");
		} else {
			channel_info_t chanInfo;
			unsignedInt chanspec;
			char chanspec_str[STRING_LENGTH_32];

			snprintf(nvram_name, sizeof(nvram_name), "%s_channel", nvifname);

			if (wl_ioctl(osifname, WLC_GET_CHANNEL, &chanInfo, sizeof(chanInfo)) < 0) {
				WIFI_ERR("%s: wl_ioctl() WLC_GET_CHANNEL failed!\n", __FUNCTION__);
				return -1;
			}
			snprintf(buf, sizeof(buf), "%d", chanInfo.target_channel);
			NVRAM_SET(nvram_name, buf);

			snprintf(nvram_name, sizeof(nvram_name), "%s_chanspec", nvifname);
			if (wl_iovar_getint(osifname, "chanspec", (int *)&chanspec) != 0) {
				WIFI_ERR("%s: Failed to get chanspec via IOCTL\n", __FUNCTION__);
				return -1;
			}
			wf_chspec_ntoa(chanspec, chanspec_str);
			NVRAM_SET(nvram_name, chanspec_str);
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		pid = get_pid_by_name(ACSD);
		if (pid <= 0) {
			WIFI_DBG("%s: acsd2 is not running, attempt to restart acsd2\n",
				__FUNCTION__);
			if (system(ACSD) == -1) {
				WIFI_ERR("%s: Couldn't start acsd2\n", __FUNCTION__);
				return -1;
			}
		}
		snprintf(nvram_name, sizeof(nvram_name), "%s_acs_boot_only", nvifname);
		nvram_value = nvram_get(nvram_name);
		if (nvram_value != NULL) {
			acs_boot_only = atoi(nvram_value) ? TRUE : FALSE;
		}

		if (*pvalue) {

			/* Set acsd2 autochannel select mode */
			snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s mode 2", ACS_CLI, osifname);
			if (system(cmdBuf) != 0) {
				WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
				return -2;
			}

			snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s autochannel", ACS_CLI, osifname);
			if (system(cmdBuf) != 0) {
				WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
				return -3;
			}

			if (acs_boot_only) {
				/* Set acsd2 disabled mode */
				snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s mode 0",
					ACS_CLI, osifname);
				if (system(cmdBuf) != 0) {
					WIFI_ERR("%s: syscmd failed for %s!\n",
						__FUNCTION__, cmdBuf);
					return -4;
				}
			}
			WIFI_DBG("%s: %s:%d, %s\n", __FUNCTION__, nvram_name, acs_boot_only,
				(acs_boot_only) ? "set acsd2 disabled mode" :
					"keep acsd2 select mode");
		} else {
			/* Set acsd2 disabled(acs_boot_only=1)) or monitor(acs_boot_only=0) mode */
			snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s mode %d",
				ACS_CLI, osifname, acs_boot_only ? 0 : 1);
			if (system(cmdBuf) != 0) {
				WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
				return -4;
			}
			WIFI_DBG("%s: %s:%d, set acsd2 %s mode\n", __FUNCTION__, nvram_name,
				acs_boot_only, (acs_boot_only) ? "disabled" : "monitor");
		}
	}

	return 0;
}

int
wldm_Radio_AutoChannelRefreshPeriod(int cmd, int radioIndex, unsigned int *pvalue,
	int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AutoChannelRefreshPeriod";
	char *nvifname, *osifname;
	char cmdBuf[BUF_SIZE] = {'\0'};
	char outBuf[BUF_SIZE] = {'\0'};
	char buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);
	if (cmd & CMD_GET) {
		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s get acs_cs_scan_timer ", ACS_CLI, osifname);

		if (syscmd(cmdBuf, outBuf, sizeof(outBuf)) != 0) {
			WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
			return -1;
		}

		if (strstr(outBuf, "info not available")) {
			WIFI_ERR("%s: acs_cs_scan_timer :  %s!\n", __FUNCTION__, outBuf);
			return -1;
		} else {
			*pvalue = strtol(outBuf, NULL, 0);
		}

		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex,
			Radio_AutoChannelRefreshPeriod_MASK);
		if (pObj == NULL)
			return -1;

		if ((*pvalue < 60) || (*pvalue > INT_MAX)) {
			WIFI_ERR("%s: acs_cs_scan_timer value out of range!\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		pObj->Radio.AutoChannelRefreshPeriod = *pvalue;
		pObj->apply_map |= Radio_AutoChannelRefreshPeriod_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_acs_cs_scan_timer", nvifname);

		snprintf(buf, sizeof(buf), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
			return -1;
		}
	}
	if (cmd & CMD_SET_IOCTL) {

		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s set acs_cs_scan_timer %d",
				ACS_CLI, osifname, *pvalue);

		if (syscmd(cmdBuf, outBuf, sizeof(outBuf)) != 0) {
			WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
			return -1;
		}
	}

	return 0;
}

int
wldm_Radio_AutoChannelDwellTime(int cmd, int radioIndex, int *pvalue,
	int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AutoChannelDwellTime";
	char *nvifname, *osifname;
	char cmdBuf[BUF_SIZE] = {'\0'};
	char outBuf[BUF_SIZE] = {'\0'};
	char buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);
	if (cmd & CMD_GET) {
		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s get acs_chan_dwell_time",
				ACS_CLI, osifname);

		if (syscmd(cmdBuf, outBuf, sizeof(outBuf)) != 0) {
			WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
			return -1;
		}

		if (strstr(outBuf, "info not available")) {
			WIFI_ERR("%s: acs_chan_dwell_time :  %s!\n", __FUNCTION__, outBuf);
			return -1;
		} else {
			*pvalue = strtol(outBuf, NULL, 0);
		}

		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		X_RDK_Radio_Object *pObj = wldm_get_X_RDK_RadioObject(radioIndex,
						X_RDK_Radio_AutoChannelDwellTime_MASK);
		if (pObj == NULL)
			return -1;

		pObj->X_RDK_Radio.AutoChannelDwellTime = *pvalue;
		pObj->apply_map |= X_RDK_Radio_AutoChannelDwellTime_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_acs_chan_dwell_time", nvifname);

		snprintf(buf, sizeof(buf), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
					__FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {

		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s set acs_chan_dwell_time %d",
				ACS_CLI, osifname, *pvalue);

		if (syscmd(cmdBuf, outBuf, sizeof(outBuf)) != 0) {
			WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
			return -1;
		}

	}

	return 0;
}

/************************
* Device.WiFi.Radio.{i}.X_COMCAST-COM_CarrierSenseThresholdInUse
************************/
int
wldm_Radio_Carrier_Sense_Threshold(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "CarrierSenseThreshold";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET  | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		*pvalue = -99;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

/**************************************************
* Device.WiFi.Radio.{i}.TransmitPowerSupported
***************************************************/
int
wldm_Radio_TransmitPowerSupported(int cmd, int radioIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "TransmitPowerSupported";
	char transmitpower[STRING_LENGTH_64] = "0,25,50,75,100";
	char *nvifname = wldm_get_radio_nvifname(radioIndex);
	char *value, nvram_name[NVRAM_NAME_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL |
				CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_txpwr_sup", nvifname);
		value = wlcsm_nvram_get(nvram_name);
		if (!value) {
			value = transmitpower;
		}
		if (*plen < (strlen(value) + 1))
			return -1;

		strcpy(pvalue, value);
		*plen = strlen(pvalue) + 1;

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	return 0;
}

/*********************************
*Device.WiFi.Radio.{i}.GuardInterval.
**********************************/

#define MAX_MCS_INDEX			WLC_MAXMCS
#define MAX_HT_MCS_INDEX(NSS)		((MAX_HT_RATES * NSS) - 1)	/* http://mcsindex.com */
#define MAX_VHT_11AC_MCS_INDEX		WLC_STD_MAX_VHT_MCS
#define MAX_VHT_11AX_EXT_MCS_INDEX	(MAX_VHT_RATES - 1)
#define MAX_HE_MCS_INDEX		MAX_HE_MCS
#define MAX_VHT_MCS_INDEX(osifname)	((is11AXCapable(osifname) < 0) ? \
					MAX_VHT_11AC_MCS_INDEX : MAX_VHT_11AX_EXT_MCS_INDEX)
int
wldm_Radio_MCS(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "MCS";
	char *osifname, *ratecmd;
	unsigned int rspec = 0, encode = 0, flags = 0, rate = 0;
	int ret, band;
	boolean override;
	int _max_ht_idx, _max_vht_idx, ntx, nss, curbw, len = sizeof(nss);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_GET_NVRAM);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "nrate", (int *) &rspec) < 0) {
			WIFI_ERR("%s: wl_iovar_getint()  nrate failed!\n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: %s nrate return rspec 0x%x\n", __FUNCTION__, osifname, rspec);

		encode = (rspec & WL_RSPEC_ENCODING_MASK);
		override =  (rspec & (WL_RSPEC_OVERRIDE_RATE | WL_RSPEC_OVERRIDE_MODE)) ? TRUE : FALSE;

		if (encode == WL_RSPEC_ENCODE_RATE) {
			*pvalue = rspec & WL_RSPEC_RATE_MASK;
		} else  if (encode == WL_RSPEC_ENCODE_HT) {
			*pvalue = rspec & WL_RSPEC_HT_MCS_MASK;
		} else if (encode == WL_RSPEC_ENCODE_VHT) {
			*pvalue = (rspec & WL_RSPEC_VHT_MCS_MASK);
		} else if (encode == WL_RSPEC_ENCODE_HE) {
			*pvalue = (rspec & WL_RSPEC_HE_MCS_MASK);
		}

		if (!override) {
			*pvalue = -1;
		}

		*plen = sizeof(*pvalue);
		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_MCS_MASK);
		if (pObj == NULL)
			return -1;

		if ((*pvalue < -1) || (*pvalue > MAX_MCS_INDEX)) {
			WIFI_ERR("%s: %d is invalid MCS index\n", __FUNCTION__, *pvalue);
			pObj->reject_map |= Radio_MCS_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}

		pObj->Radio.MCS = *pvalue;
		pObj->apply_map |= Radio_MCS_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_IOCTL) {
		if (*pvalue == -1) {
			rate = 0;
			WIFI_DBG("%s: auto\n", __FUNCTION__);
		} else {
			if (wl_iovar_getint(osifname, "nrate", (int *) &rspec) < 0) {
				WIFI_ERR("%s: wl_iovar_getint()  nrate failed!\n", __FUNCTION__);
				return -1;
			}
			encode = (rspec & WL_RSPEC_ENCODING_MASK);
			curbw = (rspec & WL_RSPEC_BW_MASK);
			if (encode == WL_RSPEC_ENCODE_HE) {
				if ((*pvalue < -1) || (*pvalue > MAX_HE_MCS_INDEX)) {
					WIFI_ERR("%s: %d mcs value out of range for HE rate!\n",
							__FUNCTION__, *pvalue);
					return -2;
				}
				rate = *pvalue & WL_RSPEC_HE_MCS_MASK;
				flags = (rspec & ~WL_RSPEC_HE_MCS_MASK);
				if (((rspec & WL_RSPEC_LDPC) == 0) && (curbw > WL_RSPEC_BW_20MHZ)) {
					WIFI_DBG("%s: HE fixup: force LDPC\n", __FUNCTION__);
					flags |= WL_RSPEC_LDPC;
				}
			} else if (encode == WL_RSPEC_ENCODE_VHT) {
				_max_vht_idx =  MAX_VHT_MCS_INDEX(osifname);
				if ((*pvalue < -1) || (*pvalue > _max_vht_idx)) {
					WIFI_ERR("%s: %d mcs value out of range for VHT rate [0 - %d]!\n",
							__FUNCTION__, *pvalue, _max_vht_idx);
					return -3;
				}
				rate = *pvalue & WL_RSPEC_VHT_MCS_MASK;
				flags = rspec & ~WL_RSPEC_VHT_MCS_MASK;
			} else if (encode == WL_RSPEC_ENCODE_HT) {
				wldm_xbrcm_Radio_TxChainMask(CMD_GET_NVRAM, radioIndex, &ntx, &len , NULL, NULL);
				_max_ht_idx = MAX_HT_MCS_INDEX(ntx);
				if ((*pvalue < -1) || (*pvalue > _max_ht_idx)) {
					WIFI_ERR("%s: %d mcs value out of range for HT rate [0 - %d]!\n",
							__FUNCTION__, *pvalue, _max_ht_idx);
					return -4;
				}
				rate = *pvalue & WL_RSPEC_HT_MCS_MASK;
				flags = rspec & (~(WL_RSPEC_HT_MCS_MASK | WL_RSPEC_TXEXP_MASK));
				if (curbw > WL_RSPEC_BW_40MHZ) {
					flags &= ~WL_RSPEC_BW_MASK;
				}
				if (*pvalue == 32) {
					nss = 1;
				} else {
					nss = 1 + (*pvalue / MAX_HT_RATES);
				}
				flags |= ((ntx - nss) << WL_RSPEC_TXEXP_SHIFT);
			} else  if (encode == WL_RSPEC_ENCODE_RATE) {
				WIFI_ERR("%s: current encode (%d) is for none MCS rate\n", __FUNCTION__, encode);
				return -5;
			}

			rate |= (flags | WL_RSPEC_OVERRIDE_RATE);
		}

		if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
			WIFI_ERR("%s: IOVAR to get band info failed\n", __FUNCTION__);
			return -6;
		}
		band = dtoh32(band);
		if (((band == WLC_BAND_AUTO) && (radioIndex == 0)) || (band == WLC_BAND_2G)) {
			ratecmd = "2g_rate";
		} else if (((band == WLC_BAND_AUTO) && (radioIndex == 1)) || (band == WLC_BAND_5G)) {
			ratecmd = "5g_rate";
                } else {
			WIFI_ERR("%s: %d is invalid band\n", __FUNCTION__, band);
			return -7;
		}

		ret = wl_iovar_setint(osifname, ratecmd, rate);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_setint %s 0x%x failed ret %d!\n",
				__FUNCTION__, ratecmd, rate, ret);
			return -8;
		}
	}

	return 0;
}

int
wldm_Radio_DfsSupport(int cmd, int radioIndex, boolean *pvalue,
	int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "DfsSupport";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		(radioIndex == 0) ? (*pvalue = FALSE) : (*pvalue = TRUE);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n",
				parameter, *pvalue ? "enabled" : "disabled");
	}
	return 0;
}

int
wldm_Radio_DfsEnable(int cmd, int radioIndex, boolean *pvalue,
	int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "DfsEnable";
	char *osifname, *nvifname;
	int ieee80211Enabled;
	char buf[BUF_SIZE] = {0};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);
	if (cmd & CMD_GET) {
		if (radioIndex == 0) {
			*pvalue = FALSE;
		} else {
			if (wl_ioctl(osifname, WLC_GET_SPECT_MANAGMENT, &ieee80211Enabled,
				sizeof(ieee80211Enabled)) < 0) {
				WIFI_ERR("%s: wl_ioctl WLC_GET_SPECT_MANAGMENT failed!\n",
					__FUNCTION__);
				return -1;
			}

			WIFI_DBG("%s: ieee80211Enabled : %d\n", __FUNCTION__, ieee80211Enabled);

			(ieee80211Enabled == 0) ? (*pvalue = FALSE) : (*pvalue = TRUE);
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "enabled" : "disabled");
	}

	if (cmd & CMD_SET) {
		X_RDK_Radio_Object *pObj = wldm_get_X_RDK_RadioObject(radioIndex,
						X_RDK_Radio_DfsEnable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->X_RDK_Radio.DfsEnable= *pvalue;
		pObj->apply_map |= X_RDK_Radio_DfsEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_reg_mode", nvifname);

		if (*pvalue)
			buf[0] = 'h';
		else
			snprintf(buf, sizeof(buf), "%s", "off");

		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		(*pvalue == 0) ? (ieee80211Enabled = 0) : (ieee80211Enabled = 1);

		if (wl_ioctl(osifname, WLC_SET_SPECT_MANAGMENT, &ieee80211Enabled,
					sizeof(ieee80211Enabled)) < 0) {
			WIFI_ERR("%s: wl_ioctl WLC_SET_SPECT_MANAGMENT failed!\n", __FUNCTION__);
			return -1;
		}
	}
	return 0;
}

static int
isValidTR181TransmitPower(int radioIndex, int TransmitPower)
{
	char supportedPowerList[STRING_LENGTH_64], *tok, *delimiter = ",";
	int len = sizeof(supportedPowerList), power;

	if (TransmitPower == -1)
		return 0;

	if (wldm_Radio_TransmitPowerSupported(CMD_GET,
		radioIndex, supportedPowerList, &len, NULL, NULL) < 0) {
		WIFI_ERR("%s: failed to get supported Transmit power\n", __FUNCTION__);
		return -1;
	}

	tok = strtok(supportedPowerList, delimiter);
	while (tok) {
		sscanf(tok, "%d", &power);
		tok = strtok(NULL, delimiter);
		if (power == TransmitPower)
			return 0;
	}

	return -2;
}

static int
wl_get_txpwr(char *osifname, uint32 *pwr_val)
{
	if (wl_iovar_getint(osifname, "qtxpower", (int *) pwr_val) < 0) {
		WIFI_ERR("%s: wl_iovar_getint() qtxpower failed!\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

int
dm_txpwr(int cmd, int radioIndex, int *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	uint32 pwr_val = 0;
	char *osifname;

	if (cmd & CMD_GET) {
		osifname = wldm_get_radio_osifname(radioIndex);
		if (wl_get_txpwr(osifname, &pwr_val) < 0) {
			WIFI_ERR("%s: wl_get_txpwr() failed!\n", __FUNCTION__);
			*pvalue = 0;
			return -1;
		}
		pwr_val &= ~WL_TXPWR_OVERRIDE;
		pwr_val /= 4;
		*pvalue = pwr_val;
	}
	return 0;
}

int
wldm_Radio_TransmitPower(int cmd, int radioIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "TransmitPower";
	char *osifname;
	uint32 pwr_val = 0, pwr_mw = 0;
	bool override = FALSE;
	int8 temp_val;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_NVRAM | CMD_GET_NVRAM);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);
	if (wl_get_txpwr(osifname, &pwr_val) < 0) {
		WIFI_ERR("%s: wl_get_txpwr() failed!\n", __FUNCTION__);
		return -1;
	}
	override = ((pwr_val & WL_TXPWR_OVERRIDE) != 0);

	if (cmd & CMD_GET) {
		pwr_val &= ~WL_TXPWR_OVERRIDE;
		temp_val = (int8)(pwr_val & 0xff);
		pwr_mw =  (temp_val < 4) ? 0 : wl_qdbm_to_mw(temp_val);
		*pvalue = math_qdiv_roundup_32((pwr_mw * 100), MAX_TRANSMIT_POWER, 0);
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex, Radio_TransmitPower_MASK);
		if (pObj == NULL)
			return -1;

		if (isValidTR181TransmitPower(radioIndex, *pvalue) < 0) {
			WIFI_ERR("%s: Invalid TransmitPower (%d)\n", __FUNCTION__, *pvalue);
			pObj->reject_map |= Radio_TransmitPower_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}

		pObj->Radio.TransmitPower = *pvalue;
		pObj->apply_map |= Radio_TransmitPower_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_IOCTL) {
		if (isValidTR181TransmitPower(radioIndex, *pvalue) < 0) {
			WIFI_ERR("%s: %d is invalid tx power percentage\n",
				__FUNCTION__, *pvalue);
			return -1;
		}

		if (*pvalue == -1) {
			override = FALSE;
			*pvalue = 100;
		}
		pwr_mw = (*pvalue) * MAX_TRANSMIT_POWER / 100;
		pwr_val = wl_mw_to_qdbm((uint16)MIN(pwr_mw, 0xffff));
		if (override)
			pwr_val |= WL_TXPWR_OVERRIDE;

		if (wl_iovar_setint(osifname, "qtxpower", (int)pwr_val) < 0) {
			WIFI_ERR("%s: wl_iovar_setint() qtxpower to %d failed!\n",
					__FUNCTION__, pwr_val);
			return -1;
		}
	}

	return 0;
}

/***********************
*  Device.WiFi.SSID.{i}.
***********************/
int
wldm_SSID_Enable(int cmd, int ssidIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Enable";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		if (wldm_AccessPoint_Enable(CMD_GET, ssidIndex,
				pvalue, plen, pbuf, pbufsz) < 0) {
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_GET_NVRAM) {
		if (wldm_AccessPoint_Enable(CMD_GET_NVRAM, ssidIndex,
				pvalue, plen, pbuf, pbufsz) < 0) {
			return -1;
		}
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_SET) {
		SSID_Object *pObj;

		pObj = wldm_get_SSIDObject(ssidIndex, SSID_Enable_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: Error Getting SSIDObject\n", __FUNCTION__);
			return -1;
		}
		pObj->Ssid.Enable = *pvalue ? TRUE : FALSE;
		pObj->apply_map |= SSID_Enable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		if (wldm_AccessPoint_Enable(CMD_SET_NVRAM, ssidIndex,
				pvalue, plen, pbuf, pbufsz) < 0) {
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		if (wldm_AccessPoint_Enable(CMD_SET_IOCTL, ssidIndex,
				pvalue, plen, pbuf, pbufsz) < 0) {
			return -1;
		}
	}

	return 0;
}

/* *plen is strlen of pvalue to SET or strlen+1 for GET */
int
wldm_SSID_SSID(int cmd, int ssidIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SSID";
	char *nvifname, *osifname, buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue!\n", __FUNCTION__, ssidIndex);
		return -1;
	}

	nvifname = wldm_get_nvifname(ssidIndex);
	osifname = wldm_get_osifname(ssidIndex);

	if (cmd & CMD_GET) {
		wlc_ssid_t *ssid = (wlc_ssid_t *)buf;

		if (wl_iovar_getbuf(osifname, "ssid", NULL, 0, buf, sizeof(buf)) < 0) {
			WIFI_ERR("%s: wl_iovar_getbuf failed!\n", __FUNCTION__);
			return -1;
		}
		if (*plen < ssid->SSID_len + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
		*plen = ssid->SSID_len;
		memcpy(pvalue, ssid->SSID, *plen);
		pvalue[*plen] = '\0';

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	if (cmd & CMD_GET_NVRAM) {
		char *ssid, nvram_name[NVRAM_NAME_SIZE];
		char *default_ssid = "Broadcom";

		snprintf(nvram_name, sizeof(nvram_name), "%s_ssid", nvifname);
		ssid = nvram_safe_get(nvram_name);
		if (*ssid == '\0') {
			WIFI_DBG("%s: %d: return default SSID %s\n",
				__FUNCTION__, ssidIndex, default_ssid);
			/* return the default value same as default in hostapd */
			ssid = default_ssid;
		}
		if  (*plen < strlen(ssid) + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, ssid);
		*plen = strlen(ssid);
	}

	if (cmd & CMD_SET) {
		SSID_Object *pObj = wldm_get_SSIDObject(ssidIndex, SSID_SSID_MASK);
		int len = strlen(pvalue);

		if (pObj == NULL)
			return -1;

		if (len > MAX_SSID_LEN) {
			WIFI_ERR("%s: Error *plen=%d\n", __FUNCTION__, *plen);
			pObj->reject_map |= SSID_SSID_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		if (pObj->Ssid.SSID) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Ssid.SSID);
			free(pObj->Ssid.SSID);
		}
		pObj->Ssid.SSID = strdup(pvalue);
		if (pObj->Ssid.SSID == NULL) {
			WIFI_ERR("%s: Error malloc failed!\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		WIFI_DBG("%s: Device.WiFi.%s.%d.%s=[%s]\n", __FUNCTION__, "SSID", ssidIndex + 1,
			parameter, pvalue);
		pObj->apply_map |= SSID_SSID_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];
		int len = strlen(pvalue);

		if (len > MAX_SSID_LEN) {
			WIFI_ERR("%s: Error *plen=%d\n", __FUNCTION__, *plen);
			return -1;
		}
		snprintf(nvram_name, sizeof(nvram_name), "%s_ssid", nvifname);
		memcpy(buf, pvalue, *plen);
		buf[*plen] = '\0';
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: Error wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		wlc_ssid_t ssid;
		int ret;

		if (*plen > MAX_SSID_LEN) {
			WIFI_ERR("%s: Error *plen=%d\n", __FUNCTION__, *plen);
			return -1;
		}
		ssid.SSID_len = *plen;
		memset((char *)ssid.SSID, 0, MAX_SSID_LEN);
		memcpy((char *)ssid.SSID, pvalue, ssid.SSID_len);
		ret = wl_ioctl(osifname, WLC_SET_SSID, &ssid, sizeof(ssid));
		if (ret != 0) {
			WIFI_ERR("%s: Error %s WLC_SET_SSID ret=%d\n", __FUNCTION__, osifname, ret);
			return -1;
		}
	}

	return 0;
}

int
wldm_SSID_MACAddress(int cmd, int ssidIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "MACAddress";
	char *osifname;
	struct ether_addr ea;
	char bssid[ETHER_ADDR_STR_LEN];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE], *nvVal;
		char *nvifname = wldm_get_nvifname(ssidIndex);
		snprintf(nvram_name, sizeof(nvram_name), "%s_hwaddr", nvifname);
		nvVal = wlcsm_nvram_get(nvram_name);
		if (nvVal == NULL || (*plen < (strlen(nvVal) + 1))) {
			WIFI_ERR("%s: wlcsm_nvram_get %s=%s !\n",
				__FUNCTION__, nvram_name, nvVal ? nvVal : "NULL");
			return -1;
		}
		strcpy(pvalue, nvVal);
		*plen = strlen(nvVal);
	}

	osifname = wldm_get_osifname(ssidIndex);

	if (cmd & CMD_GET) {
		if (WLDM_AP_DISABLED(ssidIndex)) return -1;
		/* Use "cur_etheraddr" instead of WLC_GET_BSSID */
		if (wl_iovar_get(osifname, "cur_etheraddr", &ea, sizeof(ea)) < 0) {
			WIFI_ERR("%s: ssidIndex=%d cur_etheraddr failed!\n", __FUNCTION__, ssidIndex);
			return -1;
		}

		sprintf(bssid, "%02X:%02X:%02X:%02X:%02X:%02X", (ea).octet[0], (ea).octet[1],
			(ea).octet[2], (ea).octet[3], (ea).octet[4], (ea).octet[5]);

		if (strlen(bssid) > *plen)
			return -1;

		*plen = strlen(bssid);
		strcpy(pvalue, bssid);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}
	return 0;
}

int
wldm_SSID_Status(int cmd, int ssidIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Status";
	char *osifname;
	int bss_status;
	char status[][32] = {"Disabled", "Enabled"};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(ssidIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "bss", (int *)&bss_status) < 0) {
			WIFI_ERR("%s: ssidIndex=%d wl_iovar_getint() bss failed!\n",
				__FUNCTION__, ssidIndex);
			return -1;
		}

		sprintf(pvalue, "%s", status[bss_status]);
		*plen = strlen(pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "enabled" : "disabled");
	}

	return 0;
}

int
wldm_SSID_TrafficStats(int cmd, int ssidIndex,
	Device_WiFi_SSID_Stats  *SSID_Traffic_Stats, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SSID_TrafficStats";
	char *osifname;
	wl_if_stats_v2_t *cnt;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(ssidIndex);

	if (cmd & CMD_GET) {
		cnt = (wl_if_stats_v2_t *)calloc(1, sizeof(*cnt));
		if (cnt == NULL) {
			WIFI_ERR("\tCan not allocate %d bytes for counters struct\n",
				(int)sizeof(*cnt));
			return BCME_NOMEM;
		} else {
			if (wl_iovar_get(osifname, "if_counters", cnt, sizeof(*cnt)) < 0) {
				WIFI_ERR("%s: wl_iovar_get() counters failed!\n", __FUNCTION__);
				free(cnt);
				return -1;
			}
		}

		SSID_Traffic_Stats->BytesSent			= dtoh64(cnt->txbyte);
		SSID_Traffic_Stats->BytesReceived		= dtoh64(cnt->rxbyte);
		SSID_Traffic_Stats->PacketsSent			= dtoh64(cnt->txframe);
		SSID_Traffic_Stats->PacketsReceived		= dtoh64(cnt->rxframe);
		SSID_Traffic_Stats->RetransCount		= dtoh64(cnt->txretrans);
		SSID_Traffic_Stats->RetryCount			= dtoh64(cnt->txretry);
		SSID_Traffic_Stats->MultipleRetryCount		= dtoh64(cnt->txretrie);
		SSID_Traffic_Stats->ErrorsSent			= dtoh64(cnt->txerror);
		SSID_Traffic_Stats->ErrorsReceived		= dtoh64(cnt->rxerror);
		SSID_Traffic_Stats->DiscardPacketsReceived	= dtoh64(cnt->rxnobuf);
		if (dtoh16(cnt->version) == WL_IF_STATS_VER_2) {
			SSID_Traffic_Stats->FailedRetransCount		= dtoh64(cnt->txretransfail);
			SSID_Traffic_Stats->ACKFailureCount		= dtoh64(cnt->txnoack);
			SSID_Traffic_Stats->AggregatedPacketCount	= dtoh64(cnt->txaggrpktcnt);
			SSID_Traffic_Stats->UnicastPacketsSent		= dtoh64(cnt->txucastpkts);
			SSID_Traffic_Stats->UnicastPacketsReceived	= dtoh64(cnt->rxucastpkts);
			SSID_Traffic_Stats->DiscardPacketsSent		= dtoh64(cnt->txdiscard);
			SSID_Traffic_Stats->MulticastPacketsSent	= dtoh64(cnt->txmcastpkts);
			SSID_Traffic_Stats->MulticastPacketsReceived	= dtoh64(cnt->rxmcastpkts);
			SSID_Traffic_Stats->BroadcastPacketsSent	= dtoh64(cnt->txbcastpkts);
			SSID_Traffic_Stats->BroadcastPacketsReceived	= dtoh64(cnt->rxbcastpkts);
			SSID_Traffic_Stats->UnknownProtoPacketsReceived	= dtoh64(cnt->rxbadprotopkts);
		}
		free(cnt);
	} /* if (cmd & CMD_GET) */

     return 0;
}

/* ecbd will prepare measure-interval based radio stats in tmp file */
int
wldm_Radio_TrafficStats2(int cmd, int radioIndex,
	Device_WiFi_Radio_TrafficStats2 *radio_Traffic_Stats, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RadioTrafficStats2";
	char *osifname;
	char cntbuf[WLDM_TRAFFIC_COUNTER_BUFLEN];
	const wl_cnt_wlc_t *wlc_cnt;
	const wl_cnt_v_le10_mcst_t *macstat_le10;
	const wl_cnt_lt40mcst_v1_t *macstat_lt40;
	const wl_cnt_ge40mcst_v1_t *macstat_ge40;
	wl_chanim_stats_t param;
	wl_chanim_stats_t *chanim_ptr;
	chanim_stats_v2_t *pStats_v2;
	chanim_stats_v3_t *pStats_v3;
	chanim_stats_t *pStats;
	char chan_stats_buf[WLDM_TRAFFIC_COUNTER_BUFLEN] = {0};
	time_t Systemtime;
#ifdef RDKB_RADIO_STATS_MEASURE
	char radio_stats_file[STRING_LENGTH_64];
	int ch_util, act_factor, retx_metric, cst_exc;
	FILE *fp;
#endif /* RDKB_RADIO_STATS_MEASURE */

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET  | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	if (cmd & CMD_GET) {

		if (wl_iovar_get(osifname, "counters", cntbuf, WLDM_TRAFFIC_COUNTER_BUFLEN) < 0) {
			WIFI_ERR("%s: wl_iovar_get() counters failed!\n", __FUNCTION__);
			return -1;
		}

		if (wl_cntbuf_to_xtlv_format(NULL, cntbuf, WLDM_TRAFFIC_COUNTER_BUFLEN, 0) != 0) {
			WIFI_ERR("%s: counter xtlv format failed!\n", __FUNCTION__);
			return -1;
		}

		if (!(wlc_cnt = GET_WLCCNT_FROM_CNTBUF(cntbuf))) {
			WIFI_ERR("%s: counter information extraction failed!\n", __FUNCTION__);
			return -1;
		}

		radio_Traffic_Stats->radio_BytesSent = wlc_cnt->txbyte;
		radio_Traffic_Stats->radio_BytesReceived = wlc_cnt->rxbyte;
		radio_Traffic_Stats->radio_PacketsSent = wlc_cnt->txframe;
		radio_Traffic_Stats->radio_PacketsReceived = wlc_cnt->rxframe;
		radio_Traffic_Stats->radio_ErrorsSent = wlc_cnt->txerror;
		radio_Traffic_Stats->radio_ErrorsReceived = wlc_cnt->rxerror;
		radio_Traffic_Stats->radio_DiscardPacketsSent = wlc_cnt->txnoassoc;
		radio_Traffic_Stats->radio_InvalidMACCount = wlc_cnt->rxbadsrcmac;
		radio_Traffic_Stats->radio_RetransmissionMetirc =
			wlc_cnt->txframe ? (wlc_cnt->txretrie * 100 / wlc_cnt->txframe):
			(wlc_cnt->txretrie ? 100 : 0);

		if ((macstat_le10 = MCSTCNT_LE10_FROM_CNTBUF(cntbuf)) == NULL) {
			if ((macstat_lt40 = MCSTCNT_LT40_FROM_CNTBUF(cntbuf)) == NULL) {
				if ((macstat_ge40 = MCSTCNT_GE40_FROM_CNTBUF(cntbuf)) == NULL) {
					WIFI_DBG("%s: No Counters Found in TLV\n", __FUNCTION__);
				} else {
					radio_Traffic_Stats->radio_PLCPErrorCount =
							dtoh32(macstat_ge40->rxbadplcp);
					radio_Traffic_Stats->radio_FCSErrorCount =
							dtoh32(macstat_ge40->rxbadfcs);
				}
			} else {
				radio_Traffic_Stats->radio_PLCPErrorCount = dtoh32(macstat_lt40->rxbadplcp);
				radio_Traffic_Stats->radio_FCSErrorCount = dtoh32(macstat_lt40->rxbadfcs);
			}
		} else {
			radio_Traffic_Stats->radio_PLCPErrorCount = dtoh32(macstat_le10->rxbadplcp);
			radio_Traffic_Stats->radio_FCSErrorCount = dtoh32(macstat_le10->rxbadfcs);
		}

		/* Get WL_CHANIM_COUNT_ALL ioctl values. */
		chanim_ptr = (wl_chanim_stats_t *)chan_stats_buf;
		param.buflen = htod32(WLDM_TRAFFIC_COUNTER_BUFLEN);
		param.count = htod32(WL_CHANIM_COUNT_ONE);

		if ((wl_iovar_getbuf(osifname, "chanim_stats", &param,
			sizeof(wl_chanim_stats_t), chan_stats_buf, WLDM_TRAFFIC_COUNTER_BUFLEN)) < 0) {

			WIFI_ERR("%s: chanim_stats  failed!\n", __FUNCTION__);
			return -1;
		}

		chanim_ptr->version = dtoh32(chanim_ptr->version);
		if (chanim_ptr->version < WL_CHANIM_STATS_VERSION_2) {
			WIFI_ERR("Err: chanim_stats version %d is too low\n",
				chanim_ptr->version);
			return -1;
		}

		if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_2) {
			pStats_v2 = (chanim_stats_v2_t *)chanim_ptr->stats;

			radio_Traffic_Stats->radio_NoiseFloor = pStats_v2->bgnoise;
			radio_Traffic_Stats->radio_MedianNoiseFloorOnChannel = pStats_v2->bgnoise;

			//As per CS00010404526
			radio_Traffic_Stats->radio_PacketsOtherReceived = pStats_v2->ccastats[CCASTATS_OBSS];
		} else if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_3) {
			pStats_v3 = (chanim_stats_v3_t *)chanim_ptr->stats;

			radio_Traffic_Stats->radio_NoiseFloor = pStats_v3->bgnoise;
			radio_Traffic_Stats->radio_MedianNoiseFloorOnChannel = pStats_v3->bgnoise;

			//As per CS00010404526
			radio_Traffic_Stats->radio_PacketsOtherReceived = pStats_v3->ccastats[CCASTATS_OBSS];
		} else {
			pStats = chanim_ptr->stats;
			radio_Traffic_Stats->radio_NoiseFloor = pStats->bgnoise;
			radio_Traffic_Stats->radio_MedianNoiseFloorOnChannel = pStats->bgnoise;

			//As per CS00010404526
			radio_Traffic_Stats->radio_PacketsOtherReceived = pStats->ccastats[CCASTATS_OBSS];
		}

		//As per CS00010404526
		radio_Traffic_Stats->radio_MaximumNoiseFloorOnChannel = 0x1111;
		radio_Traffic_Stats->radio_MinimumNoiseFloorOnChannel = 0x1111;
		radio_Traffic_Stats->radio_DiscardPacketsReceived = 0x1111;

#ifdef RDKB_RADIO_STATS_MEASURE /* old requirement */
		/* special radio stats from ecbd tmp file */
		ch_util = act_factor = retx_metric = -1;
		snprintf(radio_stats_file, sizeof(radio_stats_file), "/tmp/wl%d_%s",
			radioIndex, TMP_RADIO_STATS_FILE);
		fp = fopen(radio_stats_file, "r");
		if (fp) {
			fscanf(fp, "%d %d %d %d", &ch_util, &act_factor, &retx_metric, &cst_exc);
			fclose(fp);
			WIFI_DBG("%s: ch_util=%d act_factor=%d retx_metric=%d cst_exc=%d\n",
				__FUNCTION__, ch_util, act_factor, retx_metric, cst_exc);
		}
		radio_Traffic_Stats->radio_ChannelUtilization = ch_util;
		radio_Traffic_Stats->radio_ActivityFactor = act_factor;
		radio_Traffic_Stats->radio_RetransmissionMetirc = retx_metric;
		radio_Traffic_Stats->radio_CarrierSenseThreshold_Exceeded = cst_exc;
#else /* new requirement ask real-time value */
		if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_2) {
			int txop;

			pStats_v2 = (chanim_stats_v2_t *)chanim_ptr->stats;

			txop = pStats_v2->ccastats[CCASTATS_TXOP];
			txop = (txop > 100) ? 100 : txop;
			radio_Traffic_Stats->radio_ChannelUtilization = 100 - txop;
			radio_Traffic_Stats->radio_ActivityFactor = pStats_v2->ccastats[CCASTATS_TXDUR]
								+ pStats_v2->ccastats[CCASTATS_INBSS]
								+ pStats_v2->ccastats[CCASTATS_NOCTG];
			radio_Traffic_Stats->radio_CarrierSenseThreshold_Exceeded = pStats_v2->ccastats[CCASTATS_OBSS]
										+ pStats_v2->ccastats[CCASTATS_NOPKT];
		} else if (chanim_ptr->version == WL_CHANIM_STATS_VERSION_3) {
			int txop;

			pStats_v3 = (chanim_stats_v3_t *)chanim_ptr->stats;

			txop = pStats_v3->ccastats[CCASTATS_TXOP];
			txop = (txop > 100) ? 100 : txop;
			radio_Traffic_Stats->radio_ChannelUtilization = 100 - txop;
			radio_Traffic_Stats->radio_ActivityFactor = pStats_v3->ccastats[CCASTATS_TXDUR]
								+ pStats_v3->ccastats[CCASTATS_INBSS]
								+ pStats_v3->ccastats[CCASTATS_NOCTG];
			radio_Traffic_Stats->radio_CarrierSenseThreshold_Exceeded = pStats_v3->ccastats[CCASTATS_OBSS]
										+ pStats_v3->ccastats[CCASTATS_NOPKT];
		} else {
			float txdur, ibss, noctg, obss, nopkt, cu, af, cste;

			if (pStats->acc_ms) {
				txdur = dtoh32(pStats->acc_ccastats[CCASTATS_TXDUR]);
				ibss = dtoh32(pStats->acc_ccastats[CCASTATS_INBSS]);
				noctg = dtoh32(pStats->acc_ccastats[CCASTATS_NOCTG]);
				obss = dtoh32(pStats->acc_ccastats[CCASTATS_OBSS]);
				nopkt = dtoh32(pStats->acc_ccastats[CCASTATS_NOPKT]);

				cu = (txdur + ibss + noctg + obss + nopkt) / (float)(dtoh32(pStats->acc_ms)*10);
				radio_Traffic_Stats->radio_ChannelUtilization = cu + 0.5; /* round to closest integer */

				af = (txdur + ibss + noctg) / (float)(dtoh32(pStats->acc_ms)*10);
				radio_Traffic_Stats->radio_ActivityFactor = af + 0.5; /* round to closest integer */

				cste = (obss + nopkt) / (float)(dtoh32(pStats->acc_ms)*10);
				radio_Traffic_Stats->radio_CarrierSenseThreshold_Exceeded = cste + 0.5; /* round to closest integer */
			} else {
				WIFI_ERR("%s: %d: chanim_stats duration is 0\n", __FUNCTION__, radioIndex);

				radio_Traffic_Stats->radio_ChannelUtilization = 0;
				radio_Traffic_Stats->radio_ActivityFactor = 0;
				radio_Traffic_Stats->radio_CarrierSenseThreshold_Exceeded = 0;
			}
		}
#endif /* RDKB_RADIO_STATS_MEASURE */
		Systemtime = time(NULL);
		if (Systemtime == -1) {
			WIFI_DBG("%s: system time update failed!\n", __FUNCTION__);
			radio_Traffic_Stats->radio_StatisticsStartTime = 0;
		} else {
			radio_Traffic_Stats->radio_StatisticsStartTime = Systemtime;
		}

	}

     return 0;
}

//driver: 1 2 5.5 6 9 11 12 18 24 36 48 54
//bitmap: 1 2 5.5 11 6 9 12 18 24 36 48 54
bit2rate_map_t tbl_2g[] = {
	{BIT0, 2},      /* 1 Mbps */
	{BIT1, 4},      /* 2 Mbps */
	{BIT2, 11},     /* 5.5 Mbps */
	{BIT4, 12},     /* 6 Mbps */
	{BIT5, 18},     /* 9 Mbps */
	{BIT3, 22},     /* 11  Mbps */
	{BIT6, 24},     /* 12 Mbps */
	{BIT7, 36},     /* 18 Mbps */
	{BIT8, 48},     /* 24 Mbps */
	{BIT9, 72},     /* 36 Mbps */
	{BIT10, 96},    /* 48 Mbps */
	{BIT11, 108},   /* 54 Mbps */
	{BIT12, 0},     /* MCS 0 */
	{BIT13, 1},     /* MCS 1 */
	{BIT14, 2},     /* MCS 2 */
	{BIT15, 3},     /* MCS 3 */
	{BIT16, 4},     /* MCS 4 */
	{BIT17, 5},     /* MCS 5 */
	{BIT18, 6},     /* MCS 6 */
	{BIT19, 7},     /* MCS 7 */
};

bit2rate_map_t tbl_5g[] = {
	{BIT0, 12},     /* 6 Mbps */
	{BIT1, 18},     /* 9 Mbps */
	{BIT2, 24},     /* 12 Mbps */
	{BIT3, 36},     /* 18 Mbps */
	{BIT4, 48},     /* 24 Mbps */
	{BIT5, 72},     /* 36 Mbps */
	{BIT6, 96},     /* 48 Mbps */
	{BIT7, 108},    /* 54 Mbps */
	{BIT8, 0},      /* MCS 0 */
	{BIT9, 1},      /* MCS 1 */
	{BIT10, 2},     /* MCS 2 */
	{BIT11, 3},     /* MCS 3 */
	{BIT12, 4},     /* MCS 4 */
	{BIT13, 5},     /* MCS 5 */
	{BIT14, 6},     /* MCS 6 */
	{BIT15, 7},     /* MCS 7 */
};

#define BASIC_RATES_BITS_COUNT      16
#define BASIC_RATES_BITS_COUNT_2G   12
#define BASIC_RATES_BITS_COUNT_5G   8

#define SUPPORTED_RATES_BITS_COUNT  32
#define SUPPORTED_RATES_BITS_COUNT_2G   20  /* 12 legacy rates + 8 ht rates */
#define SUPPORTED_RATES_BITS_COUNT_5G   16  /* 8 legacy rate + 8 ht rates */

#define X_LGI_BASICRATES_BITMAP_STRLEN      4
#define X_LGI_SUPPORTEDRATES_BITMAP_STRLEN  8

#define BASIC_RATES_BITMASK		((1 << BASIC_RATES_BITS_COUNT) - 1)	/* 16 bits */
#define BASIC_RATES_2G_BITMASK		((1 << BASIC_RATES_BITS_COUNT_2G) - 1)	/* 12 valid bits out of 16 */
#define BASIC_RATES_5G_BITMASK		((1 << BASIC_RATES_BITS_COUNT_5G) - 1)	/* 8 valid bits out of 16 */

#define SUPPORTED_RATES_BITMASK		((1 << SUPPORTED_RATES_BITS_COUNT) - 1)	/* 32 bits */
#define SUPPORTED_RATES_2G_BITMASK	(((1 << SUPPORTED_RATES_BITS_COUNT_2G) - 1)	/* 20 bits out of 32 */
#define SUPPORTED_RATES_5G_BITMASK	((1 << SUPPORTED_RATES_BITS_COUNT_5G) - 1)	/* 16 bits out of 32 */

static int
isHexStr(char *s)
{
	int len, i;

	if (!s || *s == '\0')
		return 0;

	len = strlen(s);
	for (i = 0; i < len; i++) {
		if (isxdigit(s[i]) == 0)
			return 0;
	}

	return 1;
}

int
wldm_RatesBitmapControl_Enable(int cmd,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SupportedRatesBitmapControlEnable";
	char nvname[NVRAM_NAME_SIZE] = {0}, nvbuf[BUF_SIZE] = {0};
	char *nvvalue = NULL;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	snprintf(nvname, sizeof(nvname), "%s", WLIF_NVRAM_SUPPORT_RATE_BITMAP);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		nvvalue = nvram_safe_get(nvname);
		*pvalue = (atoi(nvvalue) == 1) ? TRUE : FALSE;

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		X_LGI_Rates_Bitmap_Control_Object *pObj = wldm_get_X_LGI_RatesControlObject(
				0, X_LGI_RATE_CONTROL_Enable_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: Error Getting X_LGI_Rates_Bitmap_Control Object\n",
					__FUNCTION__);
			return -1;
		}

		pObj->Bitmap.Enable = *pvalue;
		pObj->apply_map |= X_LGI_RATE_CONTROL_Enable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvbuf, sizeof(nvbuf), "%s",  *pvalue ? "1" : "0");
		if (wlcsm_nvram_set(nvname, nvbuf)) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
					nvname, nvbuf);
			return -1;
		}
	}

	return 0;
}

int
wldm_RatesBitmapControl_BasicRate(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RatesBitmapControlBasicRates";
	bit2rate_map_t *tbl;
	int ret, band, i, j, k, l, tbl_sz = 0, bitlen = 0, len, r, b, slen;
	boolean enable, prev_enable;
	wl_rateset_t rs;
	char *osifname = wldm_get_osifname(apIndex);
	char *nvifname = wldm_get_nvifname(apIndex);
	int radioIndex = wldm_get_radioIndex(apIndex);
	char nvName[NVRAM_NAME_SIZE], *nvValue = NULL;
	uint16 br_bitmap = 0;
	uint32 sr_bitmap = 0xffffffff;
	char *ptr;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if ((pvalue == NULL) || (*plen < X_LGI_BASICRATES_BITMAP_STRLEN)) {
	      WIFI_ERR("%s: expected *plen %d, but got %d\n", __FUNCTION__,
			X_LGI_BASICRATES_BITMAP_STRLEN, *plen);
		return -1;
	}

	if (cmd & (CMD_SET | CMD_SET_IOCTL | CMD_SET_NVRAM)) {
		if (!isHexStr(pvalue)) {
			WIFI_ERR("%s: expect hex string, but got %s\n", __FUNCTION__, pvalue);
			return -1;
		}
	}

	len = sizeof(enable);
	if (wldm_RatesBitmapControl_Enable(CMD_GET, &enable, &len, NULL, NULL) != 0) {
		WIFI_ERR("%s: wldm_RatesBitmapControl_Enable failed\n", __FUNCTION__);
		return -1;
	}

	/* when rate bitmap control feature is disabled, CMD_SET_IOCTL is not allowed */
	if (!enable && (cmd & CMD_SET_IOCTL)) {
		WIFI_ERR("%s: Skip! RatesBitmapControl feature not enabled.\n",
						__FUNCTION__);
		return -1;
	}

	if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
		WIFI_ERR("%s: IOVAR to get band info failed\n", __FUNCTION__);
		return -1;
	}
	band = dtoh32(band);

	if (((band == WLC_BAND_AUTO) && (radioIndex == 0)) ||
		(band == WLC_BAND_2G)) {
		tbl = tbl_2g;
		tbl_sz = sizeof(tbl_2g)/ sizeof(bit2rate_map_t);
		bitlen = BASIC_RATES_BITS_COUNT_2G;
	} else if (((band == WLC_BAND_AUTO) && (radioIndex == 1)) ||
		(band == WLC_BAND_5G)) {
		tbl = tbl_5g;
		tbl_sz = sizeof(tbl_5g) / sizeof(bit2rate_map_t);
		bitlen = BASIC_RATES_BITS_COUNT_5G;
	} else {
		WIFI_ERR("%s: %d is not supported band\n", __FUNCTION__, band);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_SET_IOCTL)) {
		ret = wl_ioctl(osifname, WLC_GET_CURR_RATESET, &rs, sizeof(wl_rateset_t));
		if (ret < 0) {
			WIFI_ERR("%s: wl_ioctl get current rateset failed\n", __FUNCTION__);
			return -1;
		}
	}

	if (cmd & CMD_GET) {
		if (*plen <= (sizeof(br_bitmap) * 2)) {
			WIFI_ERR("%s: input buffer %d is too short, expecting %d\n",
					__FUNCTION__, *plen, sizeof(br_bitmap)* 2);
			return -1;
		}

		br_bitmap = 0;
		for (i = 0; i < dtoh32(rs.count); i++) {
			b = rs.rates[i] & WLC_RATE_FLAG;
			if (b == 0)
				continue;

			r = rs.rates[i] & RATE_MASK;
			for (j = 0; j < bitlen ; j++) {
				if (r != tbl[j].rate)
					continue;
				br_bitmap |= 1 << tbl[j].bit;
				break;
			}
		}

		memset(pvalue, 0, *plen);
		snprintf(pvalue, *plen, "%04x", br_bitmap & BASIC_RATES_BITMASK);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	if (cmd & CMD_SET) {
		X_LGI_Rates_Bitmap_Control_Object *pObj = wldm_get_X_LGI_RatesControlObject(
						apIndex, X_LGI_RATE_CONTROL_BasicRate_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: Error Getting X_LGI_Rates_Bitmap_Control Object\n",
					__FUNCTION__);
			return -1;
		}

		if (pObj->Bitmap.BasicRatesBitMap) {
			WIFI_DBG("%s: free old BasicRatesBitMap %s\n",
					__FUNCTION__, pObj->Bitmap.BasicRatesBitMap);
			free(pObj->Bitmap.BasicRatesBitMap);
		}
		slen = strlen(pvalue) + 1;
		pObj->Bitmap.BasicRatesBitMap = malloc(slen);
		if (pObj->Bitmap.BasicRatesBitMap == NULL) {
			WIFI_ERR("%s: malloc failed!\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		snprintf(pObj->Bitmap.BasicRatesBitMap, slen, "%s", pvalue);
		pObj->apply_map |= X_LGI_RATE_CONTROL_BasicRate_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & (CMD_GET_NVRAM | CMD_SET_NVRAM)) {
		snprintf(nvName, sizeof(nvName), "%s_basic_rates_bitmap", nvifname);

		if (cmd & CMD_GET_NVRAM) {
			if (*plen <= (sizeof(br_bitmap) * 2)) {
				WIFI_ERR("%s: input buffer %d is too short, expected %d\n",
						__FUNCTION__, *plen, sizeof(br_bitmap) * 2 );
				return -1;
			}
			nvValue = wlcsm_nvram_get(nvName);
			if (!nvValue) {
				WIFI_ERR("%s: failed to get %s", __FUNCTION__, nvName);
				return -1;
			}
			snprintf(pvalue, *plen, "%s", nvValue);
		}

		if (cmd & CMD_SET_NVRAM) {
			wlcsm_nvram_set(nvName, pvalue);
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		snprintf(nvName, sizeof(nvName), "%s_supported_rates_bitmap", nvifname);
		nvValue = wlcsm_nvram_get(nvName);
		if (!nvValue) {
			WIFI_ERR("%s: failed to get nvram for %s\n", __FUNCTION__, nvName);
			return -1;
		}

		sr_bitmap = (uint32) strtoul(nvValue, &ptr, 16);
		br_bitmap = (uint16) strtoul(pvalue, &ptr, 16);
		for (j = i = 0; i < bitlen; i++) {
			if (!(sr_bitmap & (1 << i)) && (br_bitmap & (1 << i))) {
				WIFI_ERR("%s: bit %d in rate control bitmap is invalid,"
					"basic rate must also be enabled first as supported rate\n",
					__FUNCTION__, i);
				return -1;
			}

			if (sr_bitmap & (1 << i)) {
				if (wl_rateset_get_bitmap_index(tbl, tbl_sz, i, (int *)&l) < 0) {
					WIFI_ERR("%s: sr_bitmap bit %d is out of range\n", __FUNCTION__, i);
					return -1;
				}
				rs.rates[j] = tbl[l].rate;
				rs.rates[j] = (br_bitmap & (1 << i)) ?
						(rs.rates[j] | WLC_RATE_FLAG) : (rs.rates[j] & ~WLC_RATE_FLAG);
				j++;
			}
		}

		/* check for missing basic rates */
		j = 0;	/* number of basic rate count */
		k = 0;	/* number of rate count */
		for (i = 0; i < bitlen; i++) {
			if (rs.rates[i]) {
				k++;
				if (rs.rates[i] & WLC_RATE_FLAG)
					j++;
			}
		}

		if (j == 0) {
			   WIFI_ERR("%s: missing basic rates\n", __FUNCTION__);
			   return -1;
		}

		rs.count = htod32(k);

		wldm_Radio_Enable(CMD_GET, radioIndex, &prev_enable, &len, NULL, NULL);
		enable = prev_enable;
		if (prev_enable) {
			enable = 0;
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,  &enable, &len, NULL, NULL);
		}
		//error = wl_bssiovar_set(osifname, "rateset", bsscfg_idx, &defrs, rslen);
		ret = wl_ioctl(osifname, WLC_SET_RATESET, &rs, sizeof(wl_rateset_t));
		/* restore the interface up status even when rateet call is failed to avoid service interrupt */
		if (prev_enable != enable) {
			enable = 1;
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,  &enable, &len, NULL, NULL);
		}
		if (ret < 0) {
			WIFI_ERR("%s: wl_ioctl set rateset failed (%d)\n", __FUNCTION__, ret);
			return -1;
		}
	}

	return 0;
}

int
wldm_RatesBitmapControl_SupportedRate(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RatesBitmapControlSupportRates";
	wl_rateset_args_u_t rs, defrs;
	char *osifname = wldm_get_osifname(apIndex);
	char *nvifname = wldm_get_nvifname(apIndex);
	int radioIndex = wldm_get_radioIndex(apIndex);
	char nvName[NVRAM_NAME_SIZE], *nvValue = NULL;
	bit2rate_map_t *tbl;
	int ret, srbitlen, bitlen, rslen = 0, rsver = 0, band, tbl_sz, len, r, slen;
	boolean enable, prev_enable;
	int i, j, k = 0, l;
	uint32 *rscount = NULL;
	uint8 *rsrates = NULL, *rsmcs = NULL;
	uint16 *rsvht_mcs = NULL, *rshe_mcs = NULL;
	uint32 sr_bitmap = 0xffffffff;
	uint16 br_bitmap = 0;
	char *ptr;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if ((pvalue == NULL) || (*plen < X_LGI_SUPPORTEDRATES_BITMAP_STRLEN)) {
		WIFI_ERR("%s: expected *plen %d, but got %d\n", __FUNCTION__,
				X_LGI_SUPPORTEDRATES_BITMAP_STRLEN, *plen);
		return -1;
	}

	if (cmd & (CMD_SET | CMD_SET_IOCTL | CMD_SET_NVRAM)) {
		if (!isHexStr(pvalue)) {
			WIFI_ERR("%s: expect hex string, but got %s\n", __FUNCTION__, pvalue);
			return -1;
		}
	}

	len = sizeof(enable);
	if (wldm_RatesBitmapControl_Enable(CMD_GET, &enable, &len, NULL, NULL) != 0) {
		WIFI_ERR("%s: wldm_RatesBitmapControl_Enable failed\n", __FUNCTION__);
		return -1;
	}

	/* when rate bitmap control feature is disabled, CMD_SET_IOCTL is not allowed */
	if (!enable && (cmd & CMD_SET_IOCTL)) {
		WIFI_ERR("%s: Skip! RatesBitmapControl feature not enabled.\n", __FUNCTION__);
		return -1;
	}

	/* check band  */
	if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
		WIFI_ERR("%s: IOVAR to get band info failed\n", __FUNCTION__);
		return -1;
	}
	band = dtoh32(band);

	if (((band == WLC_BAND_AUTO) && (radioIndex == 0)) | (band == WLC_BAND_2G)) {
		tbl = tbl_2g;
		tbl_sz = sizeof(tbl_2g) / sizeof(bit2rate_map_t);
		srbitlen = SUPPORTED_RATES_BITS_COUNT_2G;
		bitlen = BASIC_RATES_BITS_COUNT_2G;
	} else if (((band == WLC_BAND_AUTO) && (radioIndex == 1))|| (band == WLC_BAND_5G)) {
		tbl = tbl_5g;
		tbl_sz = sizeof(tbl_5g) /sizeof(bit2rate_map_t);
		srbitlen = SUPPORTED_RATES_BITS_COUNT_5G;
		bitlen = BASIC_RATES_BITS_COUNT_5G;
	} else {
		WIFI_ERR("%s: %d is not a supported band.\n", __FUNCTION__, band);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_SET_IOCTL)) {
		if ((ret = wl_rateset_get_args_info(osifname, &rslen, &rsver)) < 0) {
			WIFI_ERR("%s: wl_rateset_get_args_info failed %d\n", __FUNCTION__, ret);
			return (ret);
		}
		wl_rateset_init_fields(&rs, rsver);
		wl_rateset_init_fields(&defrs, rsver);
	}

	if (cmd & CMD_GET) {
		if (*plen <= (sizeof(sr_bitmap) * 2)) {
			WIFI_ERR("%s: input buffer %d is too short, expected %d\n",
					__FUNCTION__, *plen, sizeof(sr_bitmap) * 2);
			return -1;
		}

		if (wl_iovar_get(osifname, "cur_rateset", &rs, rslen) < 0) {
			WIFI_ERR("%s: wl_iovar get cur_rateset failed\n", __FUNCTION__);
			return -1;
		}

		wl_rateset_get_fields(&rs, rsver, &rscount, &rsrates, &rsmcs, &rsvht_mcs,
				&rshe_mcs);

		sr_bitmap = sr_bitmap & (~((1 << srbitlen) - 1));
		for (i = 0; i < dtoh32(*rscount); i++) {
			r = rsrates[i] & RATE_MASK;
			for (j = 0; j < bitlen; j++) {
				if (r == tbl[j].rate) {
					sr_bitmap |= (1 << tbl[j].bit);
					break;
				}
			}
		}

		for (i = 0; i < (srbitlen - bitlen); i++) {
			if (isset(rsmcs, i))
				sr_bitmap |= (1 << (i +  bitlen));
		}

		memset(pvalue, 0, *plen);
		snprintf(pvalue, *plen, "%08x", sr_bitmap);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);

	}

	if (cmd & CMD_SET) {
		X_LGI_Rates_Bitmap_Control_Object *pObj = wldm_get_X_LGI_RatesControlObject(apIndex,
				X_LGI_RATE_CONTROL_SupportRate_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: Error Getting X_LGI_Rates_Bitmap_Control Object\n", __FUNCTION__);
			return -1;
		}

		if (pObj->Bitmap.SupportedRatesBitMap) {
			WIFI_DBG("%s: free old SupportedRatesBitMap %s\n",
					__FUNCTION__, pObj->Bitmap.SupportedRatesBitMap);
			free(pObj->Bitmap.SupportedRatesBitMap);
		}
		slen = strlen(pvalue) + 1;
		pObj->Bitmap.SupportedRatesBitMap = malloc(slen);
		if (pObj->Bitmap.SupportedRatesBitMap == NULL) {
			WIFI_ERR("%s: malloc failed!\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		snprintf(pObj->Bitmap.SupportedRatesBitMap, slen, "%s", pvalue);
		pObj->apply_map |= X_LGI_RATE_CONTROL_SupportRate_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & (CMD_GET_NVRAM | CMD_SET_NVRAM)) {
		snprintf(nvName, sizeof(nvName), "%s_supported_rates_bitmap", nvifname);

		if (cmd & CMD_GET_NVRAM) {
			if (*plen <= (sizeof(sr_bitmap) * 2)) {
				WIFI_ERR("%s: input buffer %d too small, expecting %d\n",
						__FUNCTION__, *plen, sizeof(sr_bitmap) * 2);
				return -1;
			}
			nvValue = wlcsm_nvram_get(nvName);
			if (!nvValue) {
				WIFI_ERR("%s: failed to get %s", __FUNCTION__, nvName);
				return -1;
			}
			snprintf(pvalue, *plen, "%s", nvValue);
		}

		if (cmd & CMD_SET_NVRAM) {
			if (*plen < len) {
				WIFI_ERR("%s: *plen (%d) is less than %d\n", __FUNCTION__, *plen, len);
				return -1;
			}
			wlcsm_nvram_set(nvName, pvalue);
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		if (wl_iovar_get(osifname, "rateset", &defrs, rslen) < 0) {
			WIFI_ERR("%s: wl_iovar get cur_rateset failed\n", __FUNCTION__);
			return -1;
		}

		wl_rateset_get_fields(&defrs, rsver, &rscount, &rsrates, &rsmcs, NULL, NULL);

		snprintf(nvName, sizeof(nvName), "%s_basic_rates_bitmap", nvifname);
		nvValue = wlcsm_nvram_get(nvName);
		if (!nvValue) {
			WIFI_ERR("%s: failed to get nvram for %s\n", __FUNCTION__, nvName);
			return -1;
		}

		br_bitmap = (uint16) strtoul(nvValue, &ptr, 16);
		sr_bitmap = (uint32) strtoul(pvalue, &ptr, 16);

		for (j = i = 0; i < bitlen; i++) {
			if (sr_bitmap & (1 << i)) {
				if (wl_rateset_get_bitmap_index(tbl, tbl_sz, i, &l) < 0) {
					WIFI_ERR("%s: sr_bitmap bit %d is out of range\n", __FUNCTION__, i);
					return -1;
				}
				rsrates[j] = tbl[l].rate;
				rsrates[j] = (br_bitmap & (1 << i)) ?
					(rsrates[j] | WLC_RATE_FLAG) : (rsrates[j] & ~WLC_RATE_FLAG);
				j++;
			}
		}

		for (i = 0; i < srbitlen - bitlen; i++) {
			(sr_bitmap & (1 << (i + bitlen))) ? setbit(rsmcs, i) :
				clrbit(rsmcs, i);
		}
		/* check for no basic rates */
		j = 0;	/* number of basic rate count */
		k = 0;	/* number of rate count */
		for (i = 0; i < bitlen; i++) {
			if (rsrates[i]) {
				k++;
				if (rsrates[i] & WLC_RATE_FLAG)
					j++;
			}
		}

		if (j == 0) {
			WIFI_ERR("%s: missing basic rates\n", __FUNCTION__);
			return -1;
		}

		*rscount = htod32(k);
		len = sizeof(enable);

		wldm_Radio_Enable(CMD_GET, radioIndex, &prev_enable, &len, NULL, NULL);
		enable = prev_enable;
		if (prev_enable) {
			enable = 0;
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,  &enable, &len, NULL, NULL);
		}
		ret = wl_iovar_set(osifname, "rateset", &defrs, rslen);
		/* restore the interface up status even when rateet call is failed to avoid service interrupt */
		if (prev_enable != enable) {
			enable = 1;
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex,  &enable, &len, NULL, NULL);
		}

		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_set rateset failed (%d) \n", __FUNCTION__, ret);
			return -1;
		}
	}

	return 0;
}

/******************************
*  Device.WiFi.AccessPoint.{i}.
******************************/
int wldm_AccessPoint_Enable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *nvifname, *osifname, buf[BUF_SIZE] = {0}, *parameter = "Enable";
	int ret = 0;
	struct { int bsscfg_idx; int enable; } bssbuf;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		if (*plen < sizeof(*pvalue)) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
		if (WLDM_AP_ENABLED(apIndex)) {

			bssbuf.bsscfg_idx = wldm_get_bssidx(apIndex);
			bssbuf.enable = WLC_AP_IOV_OP_DISABLE;
			ret = wl_iovar_getbuf(osifname, "bss", &bssbuf, sizeof(bssbuf),
					buf, sizeof(buf));
			if (ret != 0) {
				return -1;
			}
			*pvalue = *(boolean *)buf;
		} else
			*pvalue = FALSE;

		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_GET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE], *nvVal;

		snprintf(nvram_name, sizeof(nvram_name), "%s_bss_enabled", nvifname);
		nvVal = nvram_safe_get(nvram_name);
		*pvalue = atoi(nvVal) ? TRUE : FALSE;
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Object *pObj;
		pObj = wldm_get_AccessPointObject(apIndex, AccessPoint_Enable_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: Error Getting AccessPoint_Object\n", __FUNCTION__);
			return -1;
		}
		pObj->Ap.Enable = *pvalue ? TRUE : FALSE;
		pObj->apply_map |= AccessPoint_Enable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];
		snprintf(nvram_name, sizeof(nvram_name), "%s_bss_enabled", nvifname);
		snprintf(buf, sizeof(buf), "%s", *pvalue ? "1" : "0");
#ifdef BCA_CPEROUTER_RDK
		if (wlcsm_nvram_set(nvram_name, buf) != 0 ||
			wldm_mbss_nvram_set(nvifname, osifname, (boolean)*pvalue) != 0 ) {
#else
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
#endif /* BCA_CPEROUTER_RDK */
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		char scmd[BUF_SIZE] = {0};

		bssbuf.bsscfg_idx = wldm_get_bssidx(apIndex);
		bssbuf.enable = (*pvalue) ? WLC_AP_IOV_OP_ENABLE : WLC_AP_IOV_OP_DISABLE;

		ret = wl_iovar_set(osifname, "bss", &bssbuf, sizeof(bssbuf));
		if (ret != 0) {
			WIFI_ERR("%s %s ret=%d bsscfg_idx=%d\n",
				__FUNCTION__, osifname, ret, bssbuf.bsscfg_idx);
			return -1;
		}
		/* TBD - works now; move elsewhere as needed when all wldm in place */
		snprintf(scmd, sizeof(scmd), "ifconfig %s %s", osifname, (*pvalue == FALSE) ? "down" : "up");
		WIFI_DBG("%s scmd %s\n", __FUNCTION__, scmd);
		if (system(scmd) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, scmd);
		}
	}

	return 0;
}

int
wldm_AccessPoint_SSIDAdvertisementEnabled(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SSIDAdvertisementEnabled";
	char *nvifname, *osifname, buf[BUF_SIZE];
	int val;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE], *nvVal;
		snprintf(nvram_name, sizeof(nvram_name), "%s_closed", nvifname);
		nvVal = nvram_safe_get(nvram_name);

		*pvalue = (atoi(nvVal) == 0) ? TRUE : FALSE;
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_GET) {
		int ret = wl_iovar_getint(osifname, "closednet", &val);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_getint returns %d!\n", __FUNCTION__, ret);
			return -1;
		}

		/* SSIDAdvertisementEnabled is the reverse of closednet */
		*pvalue = (val == 0) ? TRUE : FALSE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "false" : "true");
	}

	if (cmd & CMD_SET) {
		AccessPoint_Object *pObj = wldm_get_AccessPointObject(apIndex,
			AccessPoint_SSIDAdvertisementEnabled_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Ap.SSIDAdvertisementEnabled = *pvalue ? 1 : 0;
		pObj->apply_map |= AccessPoint_SSIDAdvertisementEnabled_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_closed", nvifname);
		snprintf(buf, sizeof(buf), "%d", *pvalue ? 0 : 1);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		snprintf(buf, sizeof(buf), "wl -i %s closednet %d", osifname, *pvalue ? 0 : 1);
		WIFI_DBG("%s: system cmd [%s]\n", __FUNCTION__, buf);
		if (system(buf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, buf);
		}
	}

	return 0;
}

/****************************************************************
 * Device.WiFi.AccessPoint.{i}.{Isolation|Beacon Config} *
 ****************************************************************/
int
wldm_AccessPoint_IsolationEnable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "IsolationEnable";
	char *nvifname, *osifname, buf[BUF_SIZE];
	char nvram_name[NVRAM_NAME_SIZE], *nvVal;
	int val, ret, is_dhd = 0;
#ifdef __CONFIG_DHDAP__
	char *pri_osifname;
	int  radioIndex, bssidx;
#endif /* __CONFIG_DHDAP__ */

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);
#ifdef __CONFIG_DHDAP__
	radioIndex = wldm_get_radioIndex(apIndex);
	pri_osifname = wldm_get_radio_osifname(radioIndex);
        is_dhd = !dhd_probe(pri_osifname);
#endif /* __CONFIG_DHDAP__ */

	if (cmd & CMD_GET) {
#ifdef __CONFIG_DHDAP__
		if (is_dhd) {
			bssidx = wldm_get_bssidx(apIndex);
			ret = dhd_bssiovar_getint(osifname, "ap_isolate", bssidx, &val);
		} else
#endif
			ret = wl_iovar_getint(osifname, "ap_isolate", &val);

		if (ret < 0) {
			WIFI_ERR("%s: %s returns %d!\n", __FUNCTION__,
				is_dhd ? "dhd_bssiovar_getint" : "wl_iovar_getint", ret);
				return -1;
		}

		*pvalue = (val == 1) ? TRUE : FALSE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_ap_isolate", nvifname);
		nvVal = nvram_safe_get(nvram_name);
		val = atoi(nvVal);

		*pvalue = (val == 1) ? TRUE : FALSE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_SET) {
		AccessPoint_Object *pObj = wldm_get_AccessPointObject(apIndex,
			AccessPoint_IsolationEnable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Ap.IsolationEnable = *pvalue;
		pObj->apply_map |= AccessPoint_IsolationEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_ap_isolate", nvifname);
		snprintf(buf, sizeof(buf), *pvalue ? "1" : "0");
		NVRAM_SET(nvram_name, buf);
	}

	if (cmd & CMD_SET_IOCTL) {
		val = (*pvalue ? 1 : 0);

#ifdef __CONFIG_DHDAP__
		if (is_dhd) {
			bssidx = wldm_get_bssidx(apIndex);
			ret = dhd_bssiovar_setint(osifname, "ap_isolate", bssidx, val);
		} else
#endif /* __CONFIG_DHDAP__ */
			ret = wl_iovar_setint(osifname, "ap_isolate", val);

		if (ret < 0) {
			WIFI_ERR("%s: %s returns %d!\n", __FUNCTION__,
			is_dhd ? "dhd_bssiovar_setint" : "wl_iovar_getint", ret);
			return -1;
		}
	}

	return 0;
}

static int
dm_wsec(int cmd, int index, char *pvalue, int *plen, char *pvar)
{
	char *nvifname = wldm_get_nvifname(index);
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE], *pstr;

	if (strcmp(pvar, "akm") == 0) {
		pstr = "akm";
	} else if (strcmp(pvar, "auth_mode") == 0) {
		pstr = "auth_mode";
	} else {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", nvifname, pstr);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (!nvram_value) {
			WIFI_ERR("%s: %s nvram value is null\n", __FUNCTION__, nvram_name);
			return -1;
		}

		if (*plen < strlen(nvram_value) + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}

		strcpy(pvalue, nvram_value);
		*plen = strlen(pvalue) + 1;
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", nvifname, pstr);
		NVRAM_SET(nvram_name, pvalue);
	}

	return 0;
}

static int
validate_beacon_rate(char *osifname, char *input_string, char *bcn_rate)
{
	char rate[STRING_LENGTH_8] = {0}, *pbase;
	int ibeaconRate;
	boolean found_match = FALSE;
	char basicDataTransmitRates[64] = {0};

	if (!input_string)
		return -1;

	sscanf(input_string, "%[^a-zA-Z]", rate);

	if (isValidBeaconRate(rate) < 0) {
		WIFI_ERR("%s: Invalid beacon Rate %s\n", __FUNCTION__, input_string);
		return -2;
	}

	/* Check if the rate is one of the basic rate. */
	if (wl_get_current_rateset(osifname, "basic", basicDataTransmitRates) < 0) {
		WIFI_ERR("%s: wl_get_current_rateset failed\n", __FUNCTION__);
		return -3;
	}

	pbase = strtok(basicDataTransmitRates, ",");
	while (pbase != NULL) {
		if (!strcmp(pbase, rate)) {
			found_match = TRUE;
			break;
		}
		pbase = strtok(NULL, ",");
	}

	/* if beacon rate is not one of the basic rate, bail */
	if (found_match == FALSE) {
		WIFI_ERR("%s: %s is not part of the basic rates\n", __FUNCTION__, rate);
		return -4;
	}

	/* set beacon rate for bss mode */
	if (!strcmp(rate, "5.5"))
		ibeaconRate = 11;
	else
		ibeaconRate = atoi(rate) * 2;

	snprintf(bcn_rate, sizeof(ibeaconRate), "%d", ibeaconRate);

	return 0;
}

static int
dm_beacon(int cmd, int index, char *pvalue, int *plen, char *pvar)
{
	char *nvifname = wldm_get_nvifname(index);
	char *osifname = wldm_get_osifname(index);
	char nvram_name[NVRAM_NAME_SIZE], beaconRate[STRING_LENGTH_32], *iovar;
	wl_rateset_t rs;
	int bcn_rate = 0, radioIndex, len, i, ret;
	boolean is_up = FALSE, enable;

	if (strcmp(pvar, "beacon_rate") == 0) {
		iovar = "force_bcn_rspec";
	} else {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;;
	}

	if (cmd & CMD_GET) {
		unsigned int bcn_rspec;

		if (wl_iovar_getint(osifname, iovar, (int *)&bcn_rspec) != 0) {
			WIFI_ERR("%s: Failed to get beacon rate via %s\n", __FUNCTION__, iovar);
			return -1;
		}

		bcn_rate = bcn_rspec & RATE_MASK ;

		if (bcn_rate == 0) {
			ret = wl_ioctl(osifname, WLC_GET_CURR_RATESET, &rs, sizeof(wl_rateset_t));
			if (ret < 0) {
				WIFI_ERR("%s: wl_ioctl get current rateset failed\n", __FUNCTION__);
				return -3;
			}

			for (i = 0; i < rs.count; i++) {
				if (rs.rates[i] & WLC_RATE_FLAG) {
					bcn_rate = rs.rates[i] & RATE_MASK;
					break;
				}
			}
		}

		snprintf(pvalue, STRING_LENGTH_32, (bcn_rate & 0x1) ? "%d.5Mbps" : "%dMbps",  bcn_rate >> 1);
		if (*plen < strlen(pvalue) + 1) {
			WIFI_ERR("%s: buffer too short!\n", __FUNCTION__);
			return -1;
		}
		*plen = strlen(pvalue) + 1;
	}

	if (cmd & CMD_SET_NVRAM) {
		/* need to validate the beacon rate */
		if (validate_beacon_rate(osifname, pvalue, beaconRate) < 0) {
			WIFI_ERR("%s: generate_beacon_type from %s failed.\n", __FUNCTION__, pvalue);
			return -1;
		}
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", nvifname, iovar);
		NVRAM_SET(nvram_name, beaconRate);
	}

	if (cmd & CMD_SET_IOCTL) {
		if (validate_beacon_rate(osifname, pvalue, beaconRate) < 0) {
			WIFI_ERR("%s: validate_beacon_rate %s failed\n", __FUNCTION__, pvalue);
			return -1;
		}

		radioIndex = wldm_get_radioIndex(index);
		len = sizeof(boolean);

		wldm_Radio_Enable(CMD_GET, radioIndex, &is_up, &len, NULL, NULL);

		if (is_up) {
			enable = FALSE;
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex, &enable, &len, NULL, NULL);
		}

		bcn_rate = atoi(beaconRate);
		ret = wl_iovar_setint(osifname, iovar, bcn_rate);
		if (is_up) {
			enable = TRUE;
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex, &enable, &len, NULL, NULL);
		}

		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_setint %s failed set beacon rate to %d\n",
				__FUNCTION__, iovar, bcn_rate);
		}
	}

	return 0;
}

static int
get_wl_counter(int apIndex, char *cnt_name, uint32 *cnt)
{
	char *osifname = wldm_get_osifname(apIndex);
	char cmdBuf[STRING_LENGTH_128];
	char outBuf[STRING_LENGTH_128] = {0};

	snprintf(cmdBuf, sizeof(cmdBuf),
		"wl -i %s counters | grep %s | "
		"awk '{for (i = 1; i <= NF; i++) if ($(i) == \"%s\") print $(i+1)}'",
		osifname, cnt_name, cnt_name);

	if (syscmd(cmdBuf, outBuf, sizeof(outBuf)) != 0) {
		WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
		return -1;
	}

	if (outBuf[0] == 0) {
		WIFI_ERR("%s: outBuf is empty\n", __FUNCTION__);
		return -2;
	}

	*cnt = atoi(outBuf);
	return 0;
}

static int
dm_counters(int cmd, int apIndex, uint32 *pvalue, int *plen, char *pvar)
{
	uint32 cnt;
	char *parameter = pvar;

	if (cmd != CMD_GET) {
		WIFI_ERR("%s: cmd(%d) is not supported\n", __FUNCTION__, cmd);
		return -1;
	}

	if (!pvar || *plen < sizeof(uint32)) {
		WIFI_ERR("%s: invalid input for counter parameter %s\n", __FUNCTION__, parameter);
	}

	if (get_wl_counter(apIndex, pvar, &cnt) < 0) {
		WIFI_ERR("%s: unable to get counter %s value\n", __FUNCTION__, pvar);
		return -2;
	}
	*pvalue = cnt;
	*plen = sizeof(*pvalue);
	return 0;
}

static xbrcm_t xbrcm_counter_tbl[] = {
	{  "reinit",            {dm_counters},          CMD_GET,		},
	{  "reset",             {dm_counters},          CMD_GET,		},
	{  "txbcnfrm",          {dm_counters},          CMD_GET,		},
	{  NULL,                {NULL},                 0,                                                              },
};

int
wldm_xbrcm_counter(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);

	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_counter_tbl);
}

/* TR181 list consists of strings separated by comma.
 * This function converts betwen TR181 list and space-separated list.
 */
static int
convert_tr181_list(bool to_tr181, char *ptr181_list, int tr181_sz,
	char *pss_list, int ss_sz, int num_space_chars)
{
	int outlen, ret = 0;
	char *pinput, *poutput;

	if (num_space_chars < 0) {
		num_space_chars = 1; /* default 1 space */
	}

	if (to_tr181) {
		pinput = pss_list;
		poutput = ptr181_list;
		outlen = tr181_sz;

		while (*pinput == ' ') {
			pinput++; /* Skip the leading spaces */
		}

		while (*pinput && outlen > 1) {
			if (*pinput == ',') {
				pinput++; /* Skip ',' */
			} else if (*pinput == ' ') {
				if (*(poutput - num_space_chars - 1) != ',') {
					*poutput++ = ',';	/* Insert ',' */
					memset(poutput, ' ', num_space_chars);
					poutput += num_space_chars;
					outlen -= num_space_chars + 1;
				}
				pinput++; /* Skip this space */
			} else {
				*poutput++ = *pinput++;
				outlen--;
			}
		}
		*poutput = '\0';
		ret = strlen(ptr181_list);
	} else {
		pinput = ptr181_list;
		poutput = pss_list;
		outlen = ss_sz;

		if (num_space_chars <= 0) {
			num_space_chars = 1; /* At least 1 space */
		}

		while (*pinput == ' ') {
			pinput++; /* Skip the leading spaces */
		}

		while (*pinput && outlen > 1) {
			if (*pinput == ',') {
				memset(poutput, ' ', num_space_chars);
				poutput += num_space_chars;
				outlen-= num_space_chars;
				pinput++; /* Skip ',' */
			} else if (*pinput == ' ') {
				pinput++;
			} else {
				*poutput++ = *pinput++;
				outlen--;
			}
		}
		*poutput = '\0';
		ret = strlen(pss_list);
	}

	return ret;
}

/* Supported IOCTLs/IOVARs: WLC_GET_ASSOCLIST("assoclist"), WLC_GET_MACLIST/WLC_SET_MACLIST("mac"),
 *    WLC_SET_WDSLIST, "authe_sta_list", "autho_sta_list".
 */
static int
dm_maclist(int cmd, int index, char *pvalue, int *plen, char *pvar)
{
	struct maclist *list;
	int i, ret = -1, ioctl = 0, len = WLC_IOCTL_MAXLEN;
	char *pdelim, *next, *nvifname, *osifname, macstr[ETHER_ADDR_STR_LEN];

	if (pvalue == NULL || plen == NULL || pvar == NULL)
		return ret;
	list = malloc(len);
	if (list == NULL) {
		WIFI_DBG("%s: out of memory for %s!\n", __FUNCTION__, pvar);
		return ret;
	}

	nvifname = wldm_get_nvifname(index);
	osifname = wldm_get_osifname(index);
	if (cmd & CMD_SET_IOCTL) {
		if (strcmp(pvar, "maclist") == 0) {
			ioctl = WLC_SET_MACLIST;
		}

		/* Populate list from pvalue */
		list->count = 0;
		if (*pvalue == '\0' || *plen == 0) {
			len = sizeof(list->count); /* Clear the list */
		} else {
			next = pvalue;
			while (*next) {
				i = strcspn(next, ", ");
				if (i == 0) {
					next++;
					continue;
				} else if (i >= sizeof(macstr)) {
					next += i; /* Too large, skip it */
					continue;
				}
				memcpy(macstr, next, i);
				macstr[i] = '\0';
				if (ether_atoe(macstr, list->ea[list->count].octet)) {
					list->count++;
				}
				next += i;
			}
			len = list->count * ETHER_ADDR_LEN + sizeof(list->count);
		}

		if (ioctl) {
			ret = wl_ioctl(osifname, ioctl, list, len);
		} else { /* iovar */
			ret = wl_iovar_set(osifname, pvar, list, len);
		}
	}

	if (cmd & CMD_GET) {
		if (strcmp(pvar, "assoclist") == 0) {
			ret = wl_get_assoclist(osifname, (char *)list, len);
		} else if (strcmp(pvar, "maclist") == 0) {
			ret = wl_ioctl(osifname, WLC_GET_MACLIST, list, len);
		} else { /* iovar */
			ret = wl_iovar_get(osifname, pvar, list, len);
		}

		if (ret >= 0) {
			unsigned char *ea;

			memset(pvalue, 0, *plen);
			for (i = 0, len = 0; i < list->count; i++) {
				pdelim = (i == 0) ? "" : ",";
				ea = &list->ea[i].octet[0];
				len += snprintf(pvalue + len, *plen - len,
					"%s%02x:%02x:%02x:%02x:%02x:%02x", pdelim,
					ea[0], ea[1], ea[2], ea[3], ea[4], ea[5]);
				if (*plen < len + ETHER_ADDR_STR_LEN + strlen(pdelim)) {
					break;
				}
			}
			*plen = len;
			ret = 0;
		}
	}

	if (cmd & (CMD_GET_NVRAM | CMD_SET_NVRAM)) {
		char *pstr = NULL, nvname[NVRAM_NAME_SIZE], buf[BUF_SIZE];

		if (strcmp(pvar, "maclist") == 0) {
			pstr = "maclist";
		} else {
			ret = -1;
		}

		if (pstr) {
			snprintf(nvname, sizeof(nvname), "%s_%s", nvifname, pstr);
			if (cmd & CMD_GET_NVRAM) {
				pstr = wlcsm_nvram_get(nvname);
				if (pstr == NULL) {
					*pvalue = '\0';
					*plen = 1;
				} else {
					/* Convert space-separated list to TR181 list */
					convert_tr181_list(TRUE, buf, sizeof(buf),
						pstr, strlen(pstr), 1);
					len = strlen(buf) + 1;
					if (len <= *plen) {
						strcpy(pvalue, buf);
						*plen = len;
					}
				}
			} else {
				/* Convert TR181 list to space-separated list */
				convert_tr181_list(FALSE, pvalue, *plen, buf, sizeof(buf), 1);
				if (wlcsm_nvram_set(nvname, buf) != 0) {
					WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
						__FUNCTION__, nvname, buf);
					ret = -1;
				}
			}
		}
	}

	free(list);
	return ret;
}

static int
dm_int(int cmd, int index, int *pvalue, int *plen, char *pvar)
{
	char *osifname = wldm_get_osifname(index);
	char *nvifname = wldm_get_nvifname(index);
	char nvname[NVRAM_NAME_SIZE];
	int ret;

	if (!pvar) {
		WIFI_ERR("%s: input parameter is NULL!\n", __FUNCTION__);
		return -1;
	}

	if (cmd & CMD_GET) {
		ret = wl_iovar_getint(osifname, pvar, pvalue);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_getint %s returns %d!\n", __FUNCTION__, pvar, ret);
			return ret;
		}
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_SET_IOCTL) {
		ret = wl_iovar_setint(osifname, pvar, *pvalue);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_setint %s to %d returns %d!\n", __FUNCTION__, pvar, *pvalue, ret);
			return ret;
		}
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvname, sizeof(nvname), "%s_%s", nvifname, pvar);
		*pvalue = atoi(nvram_safe_get(nvname));
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_SET_NVRAM) {
		NVRAM_INT_SET(nvifname, pvar, *pvalue);
	}

	return 0;
}

static int
dm_xbrcm(int cmd, int index, void *pvalue, uint *plen, char *pvar, xbrcm_t *ptbl)
{
	int ret = -1;

	if (pvar == NULL || plen == NULL) {
		return ret;
	}
	while (ptbl->var != NULL) {
		if (strcmp(pvar, ptbl->var) == 0)
			break;
		ptbl++;
	}
	if (ptbl->var == NULL) {
		return ret;
	}

	IGNORE_CMD_WARNING(cmd, ~ptbl->cmd);

	if ((cmd & ptbl->cmd) && ptbl->dm_func) {
		ret = ptbl->dm_func(cmd, index, pvalue, plen, pvar);
	}

	return ret;
}

/* Function to call the ioctl for sta_inactivity_timeout */
static int
dm_sta_inactivity_timeout(int cmd, int radioIndex, void *pvalue, uint *plen, char *pvar)
{
	char *osifname = wldm_get_radio_osifname(radioIndex);

	UNUSED_PARAMETER(pvar);

	if (cmd & CMD_GET) {
		if (*plen < sizeof(uint)) {
			WIFI_ERR("%s: Buffer too short\n", __FUNCTION__);
			return -1;
		}
		if (wl_ioctl(osifname, WLC_GET_SCB_TIMEOUT, (int *)(pvalue), sizeof(*plen)) < 0) {
			WIFI_ERR("Err:%s:%d RSSI %s fail\n", __FUNCTION__, __LINE__, osifname);
			return -1;
		}
		*plen = sizeof(int);
	}
	return 0;
}

static xbrcm_t xbrcm_ap_tbl[] = {
	{  "assoclist",			{dm_maclist},		CMD_GET,							},
	{  "authe_sta_list",		{dm_maclist},		CMD_GET,							},
	{  "maclist",			{dm_maclist},		CMD_GET | CMD_SET_IOCTL | CMD_GET_NVRAM | CMD_SET_NVRAM,	},
	{  "maxassoc",			{NULL},			CMD_GET | CMD_SET_IOCTL | CMD_GET_NVRAM | CMD_SET_NVRAM,	},
	{  "beacon_rate",		{dm_beacon},		CMD_GET | CMD_SET_IOCTL | CMD_SET_NVRAM,			},
	{  "akm",			{dm_wsec},		CMD_GET | CMD_GET_NVRAM | CMD_SET_NVRAM,			},
	{  "auth_mode",			{dm_wsec},		CMD_GET | CMD_GET_NVRAM | CMD_SET_NVRAM,			},
	{  "mode_reqd",			{dm_mode_reqd},		CMD_GET | CMD_SET_IOCTL | CMD_GET_NVRAM | CMD_SET_NVRAM,	},
	{  "bcnprs_txpwr_offset",	{dm_int},		CMD_GET | CMD_SET_IOCTL | CMD_GET_NVRAM | CMD_SET_NVRAM,	},
	{  "sta_inactivity_timeout",	{dm_sta_inactivity_timeout},	CMD_GET,						},
	{  NULL,			{NULL},			0,								},
};

int
wldm_xbrcm_ap(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);

	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_ap_tbl);
}

/***************************************
*  Device.WiFi.AccessPoint.{i}.Security.
***************************************/
/* nvram variable 'wpa_psk' is used to store psk or passphase,
 * use the length to diffrentiate psk or passphase,
 * psk is stored in the format of 64 HEX digits, length is 64,
 * passphrase is a string that has 8..63 characters.
 */
int
wldm_AccessPoint_Security_KeyPassphrase(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "KeyPassphrase";
	char *nvifname, nvram_name[NVRAM_NAME_SIZE];
	int len;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	nvifname = wldm_get_nvifname(apIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *key_passphrase;

		if (!plen) {
			WIFI_ERR("%s: invalid null plen\n", __FUNCTION__);
			return -1;
		}

		snprintf(nvram_name, sizeof(nvram_name), "%s_wpa_psk", nvifname);
		key_passphrase = nvram_safe_get(nvram_name);
		len = strlen(key_passphrase);
		if (len < 8 || len > 63) {
			WIFI_ERR("%s: invalid wpa passphrase length [%d], expected length is [8..63]\n",
				__FUNCTION__, len);
			return -1;
		}
		if (*plen <= len) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, key_passphrase);

		if (cmd & CMD_LIST) {
			if (cmd & CMD_GET)
				PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, "xxx"); /* Hide */
			else
				PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", nvram_name, pvalue);
		}
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex,
			AccessPoint_Security_KeyPassphrase_MASK);
		if (pObj == NULL)
			return -1;

		len = strlen(pvalue);
		if (len < 8 || len > 63) {
			WIFI_ERR("%s: invalid wpa passphrase length [%d], expected length is [8..63]\n",
				__FUNCTION__, len);
			pObj->reject_map |= AccessPoint_Security_KeyPassphrase_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		if (pObj->Security.KeyPassphrase)
			free(pObj->Security.KeyPassphrase);
		pObj->Security.KeyPassphrase = strdup(pvalue);
		if (!pObj->Security.KeyPassphrase) {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		pObj->apply_map |= AccessPoint_Security_KeyPassphrase_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		len = strlen(pvalue);
		if (len < 8 || len > 63) {
			WIFI_ERR("%s: invalid wpa passphrase length [%d], expected length is [8..63]\n",
				__FUNCTION__, len);
			return -1;
		}
		snprintf(nvram_name, sizeof(nvram_name), "%s_wpa_psk", nvifname);
		if (wlcsm_nvram_set(nvram_name, pvalue) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Device_SignalStrength(int cmd, int apIndex,
	int *pvalue, int *plen, char *mac, char *pbuf, int *pbufsz)
{
	char *parameter = "SignalStrength";
	char *osifname;
	scb_val_t scb_val;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {

		memset(&scb_val, 0, sizeof(scb_val));
		if (!_wl_ether_atoe(mac, &scb_val.ea)) {
			WIFI_ERR("%s: MAC Error!\n", __FUNCTION__);
			return -1;
		}

		int ret = wl_ioctl(osifname, WLC_GET_RSSI, &scb_val, sizeof(scb_val));
		if (ret < 0) {
			WIFI_ERR("%s: wl_ioctl WLC_GET_RSSI returns %d!\n", __FUNCTION__, ret);
			return -1;
		}
		*pvalue = dtoh32(scb_val.val);
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue );
	}
	return 0;
}

int
wldm_Radio_ObssCoexistenceEnable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ObssCoexistenceEnable";
	char nvram_name[NVRAM_NAME_SIZE];
	char *osifname;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		int ret = wl_iovar_getint(osifname, "obss_coex", (int *)pvalue);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_getint returns %d!\n", __FUNCTION__, ret);
			return -1;
		}
		*pvalue = (*pvalue )? 1 : 0;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "true" : "false");
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_obss_coex", wldm_get_nvifname(apIndex));
		NVRAM_SET(nvram_name, (*pvalue != 0) ? "1" : "0");
	}

	if (cmd & CMD_SET_IOCTL) {
		/* 1=enable, 0=disable -1=Auto mode */
		if (wl_iovar_setint(osifname, "obss_coex", *pvalue)) {
			WIFI_ERR("%s: wl_iovar_setint obss_coex failed !\n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: wl_iovar_setint done !\n", __FUNCTION__);
	}

	return 0;
}

int
wldm_AccessPoint_MaxAssociatedDevices(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "MaxAssociatedDevices";
	char *nvifname, *osifname, nvram_name[NVRAM_NAME_SIZE], nv_val[BUF_SIZE];
	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_SET_NVRAM | CMD_SET_IOCTL)) {
		if (*pvalue < 0 || *pvalue > 128) {
			WIFI_ERR("%s: Value %d out of range, Valid range = 1 to 128\n",
				__FUNCTION__, *pvalue);
			return -1;
		}
	}
	if (cmd & CMD_GET) {
		if (WLDM_AP_DISABLED(apIndex)) return -1;
		if (wl_iovar_getint(osifname, "bss_maxassoc", pvalue)) {
			WIFI_ERR("%s: wl_iovar_getint bss_maxassoc failed !\n", __FUNCTION__);
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Object *pObj = wldm_get_AccessPointObject(apIndex,
						AccessPoint_MaxAssociatedDevices_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: Error getting AccessPoint_Object!\n", __FUNCTION__);
			return -1;
		}
		if (*pvalue < 0 || *pvalue > 128) {
			WIFI_ERR("%s: Value %d out of range, Valid range = 1 to 128\n",
				__FUNCTION__, *pvalue);
			pObj->reject_map |= AccessPoint_MaxAssociatedDevices_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		pObj->Ap.MaxAssociatedDevices = (unsigned int) *pvalue;
		pObj->apply_map |= AccessPoint_MaxAssociatedDevices_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_bss_maxassoc", nvifname);
		*pvalue = atoi(nvram_safe_get(nvram_name));
		if (*pvalue <= 0) {
			int radioIdx = wldm_get_radioIndex(apIndex);

			/* wlconf() sets nvram wlx_cfg_maxassoc if it is not set. */
			snprintf(nvram_name, sizeof(nvram_name), "wl%d_cfg_maxassoc", radioIdx);
			*pvalue = atoi(nvram_safe_get(nvram_name));
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_bss_maxassoc", nvifname);
		snprintf(nv_val, sizeof(nv_val), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, nv_val) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__,
				nvram_name, nv_val);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		if (wl_iovar_setint(osifname, "bss_maxassoc", *pvalue)) {
			WIFI_ERR("%s: wl_iovar_setint bss_maxassoc failed !\n", __FUNCTION__);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Status(int cmd, int apIndex,
char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Status";
	char *osifname;
	int  ap_status;
	char status[][32] = {"Disabled", "Enabled"};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "bss", (int *)&ap_status) < 0) {
			WIFI_ERR("%s: wl_iovar_getint() bss failed!\n", __FUNCTION__);
			return -1;
		}

		sprintf(pvalue, "%s", status[ap_status]);
		*plen = strlen(pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, *pvalue ? "enabled" : "disabled");
	}

	return 0;
}

int
wldm_AccessPoint_AssociatedDeviceNumber(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AssociatedDeviceNumberOfEntries";
	char *osifname;
	struct maclist *assoclist;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);
	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {

		if (WLDM_AP_DISABLED(apIndex)) return 0;

		assoclist = malloc(WLC_IOCTL_MEDLEN);
		if (!assoclist) {
			WIFI_ERR("%s: malloc failed \n", __FUNCTION__);
			return -1;
		}

		assoclist->count = htod32((WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN);
		if ((wl_ioctl(osifname, WLC_GET_ASSOCLIST, assoclist, WLC_IOCTL_MEDLEN)) < 0) {
			WIFI_WARNING("%s: wl_ioctl() WLC_GET_ASSOCLIST failed!\n", __FUNCTION__);
			free(assoclist);
			return -1;
		}
		*pvalue = dtoh32(assoclist->count);
		*plen = sizeof(*pvalue);

		free(assoclist);
		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_AccessPoint_AssocDevice(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AssociatedDevice";
	char *osifname, macaddr[ETHER_ADDR_LEN * 3];;
	struct maclist *assoclist;
	struct ether_addr *ea;
	int i,ilistlen=0;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		assoclist = malloc(WLC_IOCTL_MEDLEN);
		if (!assoclist) {
			WIFI_ERR("%s: malloc failed \n", __FUNCTION__);
			return -1;
		}

		assoclist->count = htod32((WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN);
		if ((wl_ioctl(osifname, WLC_GET_ASSOCLIST, assoclist, WLC_IOCTL_MEDLEN)) < 0) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_ASSOCLIST failed!\n", __FUNCTION__);
			free(assoclist);
			return -1;
		}
		assoclist->count = dtoh32(assoclist->count);
		for (i = 0, ea = assoclist->ea; i < assoclist->count &&
			i < (WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN; i++, ea++) {
			//printf("%s %s\n", cmd->name, wl_ether_etoa(ea));
			WIFI_DBG("%s: wl_ioctl() assoclist %s\n", __FUNCTION__,wl_ether_etoa(ea));
			strcpy(macaddr,wl_ether_etoa(ea));

			if (*plen < (ilistlen + 1)) {
				free(assoclist);
				strcpy(pvalue,"");
				WIFI_ERR("%s: not enough memory to send list !\n", __FUNCTION__);
				return -1;
			} else {
				strncat(pvalue, macaddr, *plen - ilistlen - 1);
				ilistlen += strlen(macaddr);
				if (i != (assoclist->count - 1)) {
					strncat(pvalue, ",", *plen - ilistlen - 1);
					ilistlen += strlen(",");
				}
			}
		}
		free(assoclist);
		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_AccessPoint_UAPSDCapability(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "UAPSDCapability";
	char *osifname;
	int Capability;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "wme_apsd", &Capability) < 0) {
			WIFI_ERR("%s: wl_iovar_getint wme_apsd failed !\n", __FUNCTION__);
			*pvalue = FALSE;
			return -1;
		}
		*pvalue = TRUE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_AccessPoint_UAPSDEnable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "UAPSDEnable";
	char *osifname, *nvifname;
	int UAPSDEnable;
	char  buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "wme_apsd", &UAPSDEnable) < 0) {
			WIFI_ERR("%s: wl_iovar_getint wme_apsd failed !\n", __FUNCTION__);
			return -1;
		}
		*pvalue = UAPSDEnable;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Object *pObj = wldm_get_AccessPointObject(apIndex,
					AccessPoint_UAPSDEnable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Ap.UAPSDEnable = ((*pvalue) ? 1 : 0);
		pObj->apply_map |= AccessPoint_UAPSDEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_wme_apsd", nvifname);
		snprintf(buf, sizeof(buf), "%s", *pvalue ? "on" : "off");
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {

		UAPSDEnable = ((*pvalue) ? 1 : 0);
		if (wl_iovar_setint(osifname, "wme_apsd", UAPSDEnable)) {
			WIFI_ERR("%s: wl_iovar_setint wme_apsd failed !\n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: wl_iovar_setint done !\n", __FUNCTION__);
	}

	return 0;
}

int
wldm_AccessPoint_WMMCapability(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "WMMCapability";
	char *osifname;
	int Capability;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "wme", &Capability) < 0) {
			WIFI_ERR("%s: wl_iovar_getint wme failed !\n", __FUNCTION__);
			return -1;
		}
		*pvalue = TRUE;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_AccessPoint_WMMEnable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "WMMEnable";
	char *osifname, *nvifname, *nvVal;
	int WMMEnable;
	char  buf[BUF_SIZE], nvram_name[NVRAM_NAME_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "wme", &WMMEnable) < 0) {
			WIFI_ERR("%s: wl_iovar_getint wme failed !\n", __FUNCTION__);
			return -1;
		}
		*pvalue = WMMEnable;
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_wme", nvifname);
		nvVal = nvram_safe_get(nvram_name);
		if (strcmp(nvVal, "off") == 0) {
			*pvalue = FALSE;
		} else {
			/* by default wlconf set wme to on.
			 * refer to wlconf_ampdu_amsdu_set */
			*pvalue = TRUE;
		}
	}

	if (cmd & CMD_SET) {
		AccessPoint_Object *pObj = wldm_get_AccessPointObject(apIndex,
						AccessPoint_WMMEnable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Ap.WMMEnable = ((*pvalue) ? 1 : 0);
		pObj->apply_map |= AccessPoint_WMMEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_wme", nvifname);
		snprintf(buf, sizeof(buf), "%s", *pvalue ? "on" : "off");
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {

		WMMEnable = ((*pvalue) ? 1 : 0);
		if (wl_iovar_setint(osifname, "wme", WMMEnable)) {
			WIFI_ERR("%s: wl_iovar_setint wme failed !\n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: wl_iovar_setint done !\n", __FUNCTION__);
	}

	return 0;
}

/******************************
*  Device.WiFi.Radio.i.X_CISCO_COM_CTSProtectionMode
******************************/
int
wldm_Radio_Cts_Protection_Enable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_GET | CMD_LIST |
				CMD_LIST | CMD_SET_IOCTL);

	return 0;
}

int
wldm_AccessPoint_RetryLimit(int cmd, int apIndex, unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RetryLimit";
	char *osifname;
	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		if (WLDM_AP_DISABLED(apIndex)) return -1;
		if (wl_ioctl(osifname, WLC_GET_SRL, pvalue, sizeof(*pvalue)) < 0) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_SRL failed!\n", __FUNCTION__);
			return -1;
		}
		*plen = sizeof(*pvalue);

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Object *pObj = wldm_get_AccessPointObject(apIndex, AccessPoint_RetryLimit_MASK);
		if (pObj == NULL)
			return -1;

		if (*pvalue > 255 || *pvalue < 1 ) {
			pObj->reject_map |= AccessPoint_RetryLimit_MASK;
		} else {
			pObj->Ap.RetryLimit = *pvalue;
			pObj->apply_map |= AccessPoint_RetryLimit_MASK;
		}
		wldm_rel_Object(pObj, (pObj->apply_map & AccessPoint_RetryLimit_MASK) ?
			TRUE : FALSE);
	}

	if (cmd & CMD_SET_IOCTL) {

		if (wl_ioctl(osifname, WLC_SET_SRL, pvalue, sizeof(*pvalue)) < 0) {
			WIFI_ERR("%s: wl_ioctl  WLC_GET_SRL failed !\n", __FUNCTION__);
			return -1;
		}

	}

	return 0;
}

int
wldm_AccessPoint_AclDeviceNumber(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AllowedDeviceNumber";
	char *osifname;
	struct maclist *acllist;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		acllist = malloc(WLC_IOCTL_MEDLEN);
		if (!acllist) {
			WIFI_ERR("%s: malloc failed \n", __FUNCTION__);
			return -1;
		}

		acllist->count = htod32((WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN);
		if ((wl_ioctl(osifname, WLC_GET_MACLIST, acllist, WLC_IOCTL_MEDLEN)) < 0) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_MACLIST failed!\n", __FUNCTION__);
			free(acllist);
			return -1;
		}
		*pvalue = dtoh32(acllist->count);
		*plen = sizeof(*pvalue);
		free(acllist);
		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

int
wldm_AccessPoint_MACAddressControMode(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "MACAddressControMode", nvram_name[NVRAM_NAME_SIZE];
	char *osifname, *nvifname, *macmode_val, buf[BUF_SIZE];
	int mode_setting;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		wl_ioctl(osifname, WLC_GET_MACMODE, &mode_setting, sizeof(mode_setting));
		if (mode_setting > 2) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_MACMODE failed!\n", __FUNCTION__);
			return -1;
		}
		*pvalue = mode_setting;

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_macmode", nvifname);
		macmode_val = nvram_safe_get(nvram_name);
		if (!strcmp(macmode_val, "deny")) {
			*pvalue = 1;
		} else if (!strcmp(macmode_val, "allow")) {
			*pvalue = 2;
		} else {
			*pvalue = 0;
		}
	}

	if (cmd & CMD_SET) {
		if (WLDM_AP_DISABLED(apIndex)) return -1;
		X_RDK_AccessPoint_Object *pObj = wldm_get_X_RDK_AccessPointObject(apIndex,
			X_RDK_AccessPoint_MACAddressControMode_MASK);
		if (pObj == NULL)
			return -1;

		if (*pvalue > 2 || *pvalue < 0) {
			pObj->reject_map |= X_RDK_AccessPoint_MACAddressControMode_MASK;
		} else {
			pObj->Ap.MACAddressControMode = *pvalue;
			pObj->apply_map |= X_RDK_AccessPoint_MACAddressControMode_MASK;
			WIFI_INFO("%s: CMD_SET %s macmode=%d\n", __FUNCTION__, osifname, *pvalue);
		}
		wldm_rel_Object(pObj, (pObj->apply_map &
			X_RDK_AccessPoint_MACAddressControMode_MASK) ? TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_macmode", nvifname);
		if (*pvalue == 1) {
			snprintf(buf, sizeof(buf), "%s", "deny");
		} else if (*pvalue == 2) {
			snprintf(buf, sizeof(buf), "%s", "allow");
		} else
			snprintf(buf, sizeof(buf), "%s", "disabled");

		NVRAM_SET(nvram_name, buf);
	}

	if (cmd & CMD_SET_IOCTL) {
		if (wl_ioctl(osifname, WLC_SET_MACMODE, pvalue, sizeof(*pvalue)) < 0) {
			WIFI_ERR("%s: wl_ioctl  WLC_SET_MACMODE failed !\n", __FUNCTION__);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_AclDevices(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AclDevices";
	char *osifname, macaddr[ETHER_ADDR_LEN * 3];
	struct maclist *acllist;
	struct ether_addr *ea;
	int i, ilistlen = 0;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_GET) {
		acllist = malloc(WLC_IOCTL_MEDLEN);
		if (!acllist) {
			WIFI_ERR("%s: malloc failed \n", __FUNCTION__);
			return -1;
		}

		acllist->count = htod32((WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN);
		if ((wl_ioctl(osifname, WLC_GET_MACLIST, acllist, WLC_IOCTL_MEDLEN)) < 0) {
			WIFI_ERR("%s: wl_ioctl() WLC_GET_MACLIST failed!\n", __FUNCTION__);
			free(acllist);
			return -1;
		}
		acllist->count = dtoh32(acllist->count);
		for (i = 0, ea = acllist->ea; i < acllist->count &&
			i < (WLC_IOCTL_MEDLEN - sizeof(int)) / ETHER_ADDR_LEN; i++, ea++) {
			//printf("%s %s\n", cmd->name, wl_ether_etoa(ea));
			WIFI_DBG("%s: wl_ioctl() acllist %s\n", __FUNCTION__, wl_ether_etoa(ea));
			strcpy(macaddr, wl_ether_etoa(ea));

			if (*plen < (ilistlen + 1)) {
				free(acllist);
				strcpy(pvalue, "");
				WIFI_ERR("%s: not enough memory to send list !\n", __FUNCTION__);
				return -1;
			} else {
				strncat(pvalue, macaddr, *plen - ilistlen - 1);
				ilistlen += strlen(macaddr);
				if (i != (acllist->count - 1)) {
					strncat(pvalue, "\n", *plen - ilistlen - 1);
					ilistlen += strlen("\n");
				}
			}

		}

		free(acllist);
		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

static int
wl_chkandkickAclDevice(int apIndex, char *staMacAddress, int macOper)
{
	int macmode, ret, len;

	if ((macOper != CMD_ADD) && (macOper != CMD_DEL)) {
		WIFI_ERR("%s: apIndex=%d no kick macOper=%d\n", __FUNCTION__, apIndex, macOper);
		return 0;
	}

	len = sizeof(macmode);
	ret = wldm_AccessPoint_MACAddressControMode(CMD_GET, apIndex, &macmode, &len, NULL, NULL);
	if (ret < 0) {
		WIFI_ERR("%s: apIndex=%d get macmode fail ret=%d\n", __FUNCTION__, apIndex, ret);
		return -1;
	}

	if ((macmode != WLC_MACMODE_ALLOW) && (macmode != WLC_MACMODE_DENY)) {
		return 0;
	}

	/* check allow and remove OR deny and add */
	if (((macmode == WLC_MACMODE_ALLOW) && (macOper == CMD_DEL)) ||
	    ((macmode == WLC_MACMODE_DENY) && (macOper == CMD_ADD))) {
		len = ETHER_ADDR_LEN * 3; /* XX:XX:XX:XX:XX:XX */
		ret = wldm_AccessPoint_kickAssociatedDevice(CMD_SET_IOCTL, apIndex, staMacAddress,
			&len, NULL, NULL);
		if (ret != 0) {
			WIFI_ERR("%s: apIndex=%d Unable to kick STA device %s returns %d\n",
				__FUNCTION__, apIndex, staMacAddress, ret);
			return -1;
		}
		WIFI_DBG("%s: apIndex=%d macmode=%d kick STA device %s\n", __FUNCTION__,
			apIndex, macmode, staMacAddress);
	}
	return 0;
}

int
wldm_AccessPoint_AclDevice(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AclDevice";
	char *osifname, *nvifname;
	char nvram_name[NVRAM_NAME_SIZE], *p_nvram_val = NULL, *p_old_nvram_val = NULL;
	struct maclist *maclist = NULL;
	struct ether_addr ea;
	uint i, len = 0;
	uint max = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
	bool found;
	int ret = 0;
	const unsigned int MAX_NUM_OF_ACL_ENTRIES = 20;
	static unsigned int g_max_num_of_acl_entries = ~0;

	IGNORE_CMD_WARNING(cmd, CMD_GET | CMD_SET | CMD_SET_IOCTL | CMD_SET_NVRAM);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	WIFI_DBG("%s: %d: cmd=0x%x\n", __FUNCTION__, apIndex, cmd);

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	/* Add to or remove from driver */
	if (!_wl_ether_atoe(pvalue, &ea)) {
		WIFI_ERR("%s: %d: MAC Error\n", __FUNCTION__, apIndex);
		return -1;
	}
	maclist = malloc(WLC_IOCTL_MAXLEN);
	if (!maclist) {
		WIFI_ERR("%s: %d: malloc failed\n", __FUNCTION__, apIndex);
		return -1;
	}

	/* Get old list */
	maclist->count = htod32(max);
	if ((wl_ioctl(osifname, WLC_GET_MACLIST, maclist, WLC_IOCTL_MAXLEN)) < 0) {
		WIFI_ERR("%s: IOCTL WLC_GET_MACLIST failed!\n", __FUNCTION__);
		ret = -1;
		goto RET;
	}
	maclist->count = dtoh32(maclist->count);

	/* Get maximum allowed number of ACL entries from nvram */
	if ((cmd & CMD_ADD) && (g_max_num_of_acl_entries == ~0)) {
		g_max_num_of_acl_entries = atoi(nvram_safe_get("wl_maclist_max_num"));

		if (g_max_num_of_acl_entries == 0) {
			g_max_num_of_acl_entries = MAX_NUM_OF_ACL_ENTRIES;
			WIFI_ERR("%s: %d: max num of ACL entries is not defined in nvram, "
				"use the default max value [%d]\n",
				__FUNCTION__, apIndex, g_max_num_of_acl_entries);
		}
	}

	if (cmd & CMD_ADD) {
		if (maclist->count >= g_max_num_of_acl_entries || maclist->count >= max) {
			WIFI_ERR("%s: %d: CMD_ADD: "
				"num of ACL entries in driver hits the maximum [%d]\n",
				__FUNCTION__, apIndex, maclist->count);
			ret = -1;
			goto RET;
		}

		found = FALSE;
		for (i = 0; i < maclist->count; i++) {
			if (eacmp(&ea, &(maclist->ea[i])) == 0) {
				found = TRUE;
				break;
			}
		}

		if (!found) {
			memcpy(&(maclist->ea[maclist->count]), &ea, ETHER_ADDR_LEN);
			maclist->count++;
			len = sizeof(maclist->count) + maclist->count * sizeof(maclist->ea);
			maclist->count = htod32(maclist->count);
			if (wl_ioctl(osifname, WLC_SET_MACLIST, maclist, len) < 0) {
				WIFI_ERR("%s: %d: CMD_ADD: ioctl WLC_SET_MACLIST failed\n",
					__FUNCTION__, apIndex);
				ret = -1;
				goto RET;
			}
		} else {
			WIFI_ERR("%s: %d: CMD_ADD: entry already exists in driver\n", __FUNCTION__, apIndex);
		}
		wl_chkandkickAclDevice(apIndex, pvalue, CMD_ADD);
	} else if (cmd & CMD_DEL) {
		found = FALSE;
		for (i = 0; i < maclist->count; i++) {
			if (eacmp(&ea, &(maclist->ea[i])) == 0) {
				memcpy(&maclist->ea[i],
				&maclist->ea[maclist->count-1], ETHER_ADDR_LEN);
				maclist->count--;
				found = TRUE;
				break;
			}
		}

		if (found) {
			len = sizeof(maclist->count) + maclist->count * sizeof(maclist->ea);
			maclist->count = htod32(maclist->count);
			if (wl_ioctl(osifname, WLC_SET_MACLIST, maclist, len) < 0) {
				WIFI_ERR("%s: wl_ioctl WLC_SET_MACLIST failed !\n", __FUNCTION__);
				ret = -1;
				goto RET;
			}
		} else {
			WIFI_ERR("%s: %d: CMD_DEL: entry not found in driver\n", __FUNCTION__, apIndex);
		}

		wl_chkandkickAclDevice(apIndex, pvalue, CMD_DEL);
	}

	/* Add to or remove from NVRAM */
	snprintf(nvram_name, sizeof(nvram_name), "%s_maclist", nvifname);
	p_old_nvram_val = nvram_safe_get(nvram_name);

	if (cmd & CMD_ADD) { /* TODO: Add and implement CMD_ADD_NVRAM. */
		if (find_in_list(p_old_nvram_val, pvalue) == NULL) {
			len = strlen(pvalue) + 1;

			if ((strlen(p_old_nvram_val) + 1) / 18 >= g_max_num_of_acl_entries) {
				WIFI_ERR("%s: %d: CMD_ADD:"
					"num of ACL entries in nvram hits the maximum [%d]\n",
					__FUNCTION__, apIndex, g_max_num_of_acl_entries);
				goto RET;
			}

			if (p_old_nvram_val[0] != '\0') {
				len += (strlen(p_old_nvram_val) + 1); /* 1 for space */
			}

			p_nvram_val = malloc(len);
			if (!p_nvram_val) {
				ret = -1;
				goto RET;
			}

			if (p_old_nvram_val[0] != '\0') {
				snprintf(p_nvram_val, len, "%s %s", p_old_nvram_val, pvalue);
			} else {
				strcpy(p_nvram_val, pvalue);
			}

			/* nvram will check if the value length exceeds the maximum */
			if (nvram_set(nvram_name, p_nvram_val) != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
						__FUNCTION__, nvram_name, p_nvram_val);
				ret = -1;
				goto RET;
			}
		} else {
			WIFI_ERR("%s: %d: CMD_ADD: entry is already in the nvram\n",
				__FUNCTION__, apIndex);
		}
	} else if (cmd & CMD_DEL) { /* TODO: Add and implement CMD_DEL_NVRAM. */
		ret = remove_from_list(pvalue, p_old_nvram_val, strlen(p_old_nvram_val));
		if (ret == 0) {
			if (nvram_set(nvram_name, p_old_nvram_val) != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
						__FUNCTION__, nvram_name, p_old_nvram_val);
				ret = -1;
				goto RET;
			}
		} else {
			WIFI_ERR("%s: %d: CMD_DEL: entry not found in the nvram\n",
				__FUNCTION__, apIndex);
		}
	}

RET:
	if (maclist)
		free(maclist);

	if (p_nvram_val)
		free(p_nvram_val);

	return ret;
}

int
wldm_AccessPoint_kickAssociatedDevice(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "KickAssociatedMac";
	char *osifname;
	struct ether_addr sta_addr;

	IGNORE_CMD_WARNING(cmd, CMD_GET | CMD_SET | CMD_DEL | CMD_SET_NVRAM);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}
	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_SET_IOCTL) {
		if (!_wl_ether_atoe(pvalue, &sta_addr)) {
			WIFI_ERR("%s: MAC Error!\n", __FUNCTION__);
			return -1;
		}
		if (wl_ioctl(osifname, WLC_SCB_DEAUTHENTICATE, &sta_addr, ETHER_ADDR_LEN) < 0) {
			WIFI_ERR("%s: wl_ioctl  WLC_SCB_DEAUTHENTICATE failed !\n", __FUNCTION__);
			return -1;
		}
	}

	return 0;
}

/* Deletes all Device MAC address from the Access control filter list */
int
wldm_AccessPoint_DelAclDevices(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "DelAclDevices";
	char *osifname, *nvifname, buf[BUF_SIZE];
	IGNORE_CMD_WARNING(cmd, CMD_GET | CMD_SET | CMD_SET_IOCTL | CMD_SET_NVRAM);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_DEL) {
		char nvram_name[NVRAM_NAME_SIZE];
		struct maclist *acllist;

		memset(buf, 0, BUF_SIZE);
		snprintf(nvram_name, sizeof(nvram_name), "%s_maclist", nvifname);
		NVRAM_SET(nvram_name, buf);

		acllist = malloc(WLC_IOCTL_MEDLEN);
		if (!acllist) {
			WIFI_ERR("%s: malloc failed \n", __FUNCTION__);
			return -1;
		}
		acllist->count = htod32(0);
		if (wl_ioctl(osifname, WLC_SET_MACLIST, acllist, WLC_IOCTL_MEDLEN) < 0) {
			WIFI_ERR("%s: wl_ioctl  WLC_SET_MACLIST failed !\n", __FUNCTION__);
			free(acllist);
			return -1;
		}
		free(acllist);
	}

	return 0;
}

int
wldm_AccessPoint(int cmd, int apIndex, char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	boolean enable = FALSE;
	char *osifname;
	int bssidx, len;

	IGNORE_CMD_WARNING(cmd, CMD_LIST | CMD_GET | CMD_SET | CMD_GET_NVRAM | CMD_SET_IOCTL);

	osifname = wldm_get_osifname(apIndex);
	bssidx = wldm_get_bssidx(apIndex);

	if (cmd & CMD_ADD) {
		int radioIdx = wldm_get_radioIndex(apIndex);
		char *radio_osifname = wldm_get_radio_osifname(radioIdx);
		wlc_ssid_t iovSsid = { 0, {0} };
		int ssidIndex;
		char ssid[MAX_SSID_LEN + 1];

		if (sscanf(pvalue, "Device.WiFi.SSID.%d.", &ssidIndex) == 1 &&
			(ssidIndex >= 1 && ssidIndex <= wldm_get_max_aps())) {
			/* SSIDReference pathname is given, get the SSID */
			ssidIndex -= 1;
			len = sizeof(ssid);
			wldm_SSID_SSID(CMD_GET_NVRAM, ssidIndex, ssid, &len, NULL, NULL);
			len = strlen(ssid);
		} else {
			len = strlen(pvalue);
			if (len > MAX_SSID_LEN) {
				WIFI_ERR("%s: SSID is bigger than 32 bytes\n", __FUNCTION__);
				return -1;
			}
			strncpy(ssid, pvalue, sizeof(ssid));
		}
		iovSsid.SSID_len = len;
		memcpy(iovSsid.SSID, ssid, iovSsid.SSID_len);
		if (wl_bssiovar_set(radio_osifname, "ssid", bssidx, &iovSsid, sizeof(iovSsid))) {
			WIFI_ERR("%s: wl_bssiovar_set [%d] failed [%s] radio=[%s]\n",
				__FUNCTION__, bssidx, strerror(errno), radio_osifname);
			return -1;
		}

		if (cmd & CMD_SET_NVRAM) {
			wldm_SSID_SSID(CMD_SET_NVRAM, apIndex, ssid, &len, NULL, NULL);
		}

		enable = TRUE;
		len = sizeof(enable);
		if (wldm_AccessPoint_Enable(CMD_SET, apIndex,
			&enable, &len, pbuf, pbufsz) < 0) {
			WIFI_ERR("%s:%d wldm_AccessPoint_Enable failed %s!\n",
				__FUNCTION__, __LINE__, osifname);
			return -1;
		}
	} else if (cmd & CMD_DEL) {
		enable = FALSE;
		len = sizeof(enable);
		if (wldm_AccessPoint_Enable(CMD_SET, apIndex,
			&enable, &len, pbuf, pbufsz) < 0) {
			WIFI_ERR("%s:%d wldm_AccessPoint_Enable failed %s!\n",
					__FUNCTION__, __LINE__, osifname);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Basic_Authenticationmode(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Authenticationmode";
	char *osifname, *nvifname, nvram_name[NVRAM_NAME_SIZE];
	char buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL );

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & (CMD_GET )) {
		int authmode, ret;
		if (WLDM_AP_DISABLED(apIndex)) return -1;
		ret = wl_iovar_getint(osifname, "wpa_auth", &authmode);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_getint returns %d!\n", __FUNCTION__, ret);
			return -1;
		}
		if (authmode == 0x80 || authmode == 0x84 || authmode == 0x04) {
			strncpy(pvalue, basicAuthenticationModeStr[BASIC_AUTHENTICATION_MODE_PSK],
				strlen(basicAuthenticationModeStr[BASIC_AUTHENTICATION_MODE_PSK]));
		}
		else if (authmode == 0x40 || authmode == 0x42) {
			strncpy(pvalue, basicAuthenticationModeStr[BASIC_AUTHENTICATION_MODE_EAP],
				strlen(basicAuthenticationModeStr[BASIC_AUTHENTICATION_MODE_EAP]));
		} else {
			strncpy(pvalue, basicAuthenticationModeStr[BASIC_AUTHENTICATION_MODE_NONE],
				strlen(basicAuthenticationModeStr[BASIC_AUTHENTICATION_MODE_NONE]));
		}

	}

	if (cmd == CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_BasicAuthmode_MASK);
		if (pObj == NULL)
			return -1;

		if ((strncmp(pvalue, "WPA-Personal", strlen("WPA-Personal")) != 0) &&
			(strncmp(pvalue, "WPA2-Personal", strlen("WPA2-Personal")) != 0) &&
			(strncmp(pvalue, "WPA-WPA2-Personal", strlen("WPA-WPA2-Personal")) != 0) &&
			(strncmp(pvalue, "WPA2-Enterprise", strlen("WPA2-Enterprise")) != 0) &&
			(strncmp(pvalue, "WPA-WPA2-Enterprise", strlen("WPA-WPA2-Enterprise")) != 0) &&
			(strncmp(pvalue, "None", strlen("None")) != 0))
				pObj->reject_map |= X_RDK_AccessPoint_Security_BasicAuthmode_MASK;
		else {
			if (pObj->Security.BasicAuthentication)
				free(pObj->Security.BasicAuthentication);
			pObj->Security.BasicAuthentication = malloc(strlen(pvalue) + 1);
			if (pObj->Security.BasicAuthentication == NULL) {
				WIFI_ERR("%s malloc failed\n", __FUNCTION__);
				wldm_rel_Object(pObj, FALSE);
				return -1;
			}
			strcpy(pObj->Security.BasicAuthentication, pvalue);
			pObj->apply_map |= X_RDK_AccessPoint_Security_BasicAuthmode_MASK;
		}
		WIFI_DBG("%s: %d %s!\n", __FUNCTION__, *plen ,pObj->Security.BasicAuthentication);
		wldm_rel_Object(pObj, (pObj->apply_map &
			X_RDK_AccessPoint_Security_BasicAuthmode_MASK) ? TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {

		WIFI_DBG("%s: CMD_SET_NVRAM %s!\n", __FUNCTION__ ,pvalue);
		snprintf(nvram_name, sizeof(nvram_name), "%s_akm", nvifname);
		if (strncmp(pvalue, "WPA-Personal", strlen("WPA-Personal")) == 0) {
			snprintf(buf, sizeof(buf),"%s", WL_AUTH_WPA_PSK);
		} else if (strncmp(pvalue, "WPA2-Personal", strlen("WPA2-Personal")) == 0) {
			snprintf(buf, sizeof(buf),"%s", WL_AUTH_WPA2_PSK);
		} else if (strncmp(pvalue, "WPA-WPA2-Personal", strlen("WPA-WPA2-Personal")) == 0) {
			snprintf(buf, sizeof(buf),"%s", WL_AUTH_WPA2_PSK_MIX);
		} else if (strncmp(pvalue, "WPA2-Enterprise", strlen("WPA2-Enterprise")) == 0) {
			snprintf(buf, sizeof(buf), "%s", WL_AUTH_WPA);
		} else if (strncmp(pvalue, "WPA-WPA2-Enterprise", strlen("WPA-WPA2-Enterprise")) == 0) {
			snprintf(buf, sizeof(buf), "%s", WL_AUTH_WPA2_MIX);
		} else if  (strncmp(pvalue, "None", strlen("None")) == 0) {
                        snprintf(buf, sizeof(buf), "%s", "None");
		}
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, pvalue);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		WIFI_DBG("%s: CMD_SET_IOCTL %s!\n", __FUNCTION__ ,pvalue);
		/*set both wpa_auth and wsec*/
		if (strncmp(pvalue, "WPA-Personal", strlen("WPA-Personal")) == 0) {
			if (wl_iovar_setint(osifname, "wpa_auth", 0x04)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 68)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if (strncmp(pvalue, "WPA2-Personal", strlen("WPA2-Personal")) == 0) {
			if (wl_iovar_setint(osifname, "wpa_auth", 0x80)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 68)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if (strncmp(pvalue, "WPA-WPA2-Personal", strlen("WPA-WPA2-Personal")) == 0) {

			if (wl_iovar_setint(osifname, "wpa_auth", 0x84)) {
				 WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 70)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}

		} else if (strncmp(pvalue, "WPA2-Enterprise", strlen("WPA2-Enterprise")) == 0) {

			if (wl_iovar_setint(osifname, "wpa_auth", 0x40)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 70)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if (strncmp(pvalue, "WPA-WPA2-Enterprise", strlen("WPA-WPA2-Enterprise")) == 0) {
			if (wl_iovar_setint(osifname, "wpa_auth", 0x42)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 70)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if  (strncmp(pvalue, "None", strlen("None")) == 0) {

			if (wl_iovar_setint(osifname, "wpa_auth", 0x00)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 64)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if  (strncmp(pvalue, "WEP-128", strlen("WEP-128")) == 0) {

			if (wl_iovar_setint(osifname, "wpa_auth", 0x01)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 1)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		}
	}

	return 0;
}

const char *wldm_supported_security_modes[] = {
	"None",
	//"WEP-64",
	//"WEP-128",
	"WPA-Personal",
	"WPA2-Personal",
	"WPA3-Personal",
	"WPA-WPA2-Personal",
	"WPA3-Personal-Transition",
	"WPA-Enterprise",
	"WPA2-Enterprise",
	"WPA3-Enterprise",
	"WPA-WPA2-Enterprise"
};

int
wldm_AccessPoint_Security_ModeEnabled(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ModeEnabled";
	char *osifname, *nvifname, nvram_name[NVRAM_NAME_SIZE];
	char *mode = NULL, buf[BUF_SIZE] = {0};
	int i, num;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL );

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue!\n", __FUNCTION__);
		return -1;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		int authmode = WPA_AUTH_DISABLED;
		char *akm_val, *next, tmp[128];

		if (!plen) {
			WIFI_ERR("%s: invalid null plen!\n", __FUNCTION__);
			return -1;
		}

		snprintf(nvram_name, sizeof(nvram_name), "%s_akm", nvifname);
		akm_val = nvram_safe_get(nvram_name);
		foreach(tmp, akm_val, next) {
			if (!strcmp(tmp, "psk"))
				authmode |= WPA_AUTH_PSK;
			else if (!strcmp(tmp, "psk2"))
				authmode |= WPA2_AUTH_PSK;
			else if (!strcmp(tmp, "sae"))
				authmode |= WPA3_AUTH_SAE_PSK;
			else if (!strcmp(tmp, "wpa"))
				authmode |= WPA_AUTH_UNSPECIFIED;
			else if (!strcmp(tmp, "wpa2")) {
				authmode |= WPA2_AUTH_UNSPECIFIED;
			}
			else {
				WIFI_DBG("%s: undefined or unsupported akm [%s]!\n", __FUNCTION__, tmp);
			}
		}

		if (authmode == WPA_AUTH_DISABLED) {
			mode = "None";
		} else if (authmode == WPA_AUTH_PSK) {
			mode = "WPA-Personal";
		} else if (authmode == WPA2_AUTH_PSK) {
			mode = "WPA2-Personal";
		} else if (authmode == WPA3_AUTH_SAE_PSK) {
			mode = "WPA3-Personal";
		} else if (authmode == (WPA_AUTH_PSK | WPA2_AUTH_PSK)) {
			mode = "WPA-WPA2-Personal";
		} else if (authmode == (WPA2_AUTH_PSK | WPA3_AUTH_SAE_PSK)) {
			mode = "WPA3-Personal-Transition";
		} else if (authmode == WPA_AUTH_UNSPECIFIED) {
			mode = "WPA-Enterprise";
		} else if (authmode == WPA2_AUTH_UNSPECIFIED) {
			/* WPA3-Enterprise: akm=wpa2, mfp=2 */
			int mfp = 0;

			snprintf(nvram_name, sizeof(nvram_name), "%s_mfp", nvifname);
			mfp = atoi(nvram_safe_get(nvram_name));

			if (mfp == 2)
				mode = "WPA3-Enterprise";
			else
				mode = "WPA2-Enterprise";
		} else if (authmode == (WPA_AUTH_UNSPECIFIED | WPA2_AUTH_UNSPECIFIED)) {
			mode = "WPA-WPA2-Enterprise";
		} else {
			WIFI_ERR("%s: unsupported akm [%s]!\n", __FUNCTION__, akm_val);
			return -1;
		}

		if (*plen <= strlen(mode)) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, mode);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex, AccessPoint_Security_ModeEnabled_MASK);
		if (pObj == NULL)
			return -1;

		num = sizeof(wldm_supported_security_modes)/sizeof(wldm_supported_security_modes[0]);
		for (i = 0; i < num; ++i) {
			if (strcmp(pvalue, wldm_supported_security_modes[i]) == 0)
				break;
		}
		if (i == num) {
			WIFI_ERR("%s unsupported security mode [%s]\n", __FUNCTION__, pvalue);
			pObj->reject_map |= AccessPoint_Security_ModeEnabled_MASK;
		} else {
			if (pObj->Security.ModeEnabled)
				free(pObj->Security.ModeEnabled);
			pObj->Security.ModeEnabled = malloc(strlen(pvalue) + 1);
			if (pObj->Security.ModeEnabled) {
				strcpy(pObj->Security.ModeEnabled, pvalue);
				pObj->apply_map |= AccessPoint_Security_ModeEnabled_MASK;
			} else {
				WIFI_ERR("%s malloc failed\n", __FUNCTION__);
				wldm_rel_Object(pObj, FALSE);
				return -1;
			}
			WIFI_DBG("%s: CMD_SET %s\n", __FUNCTION__ ,pvalue);
		}
		wldm_rel_Object(pObj, (pObj->apply_map & AccessPoint_Security_ModeEnabled_MASK) ?
			TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {
		num = sizeof(wldm_supported_security_modes)/sizeof(wldm_supported_security_modes[0]);
		for (i = 0; i < num; ++i) {
			if (strcmp(pvalue, wldm_supported_security_modes[i]) == 0)
				break;
		}
		if (i == num) {
			WIFI_ERR("%s unsupported security mode [%s]\n", __FUNCTION__, pvalue);
			return -1;
		}

		snprintf(nvram_name, sizeof(nvram_name), "%s_akm", nvifname);
		if (strcmp(pvalue, "None") == 0) {
			buf[0] = '\0';
		} else if (strcmp(pvalue, "WPA-Personal") == 0) {
			snprintf(buf, sizeof(buf), "%s", "psk");
		} else if (strcmp(pvalue, "WPA2-Personal") == 0) {
			snprintf(buf, sizeof(buf), "%s", "psk2");
		} else if (strcmp(pvalue, "WPA3-Personal") == 0) {
			snprintf(buf, sizeof(buf), "%s", "sae");
		} else if (strcmp(pvalue, "WPA-WPA2-Personal") == 0) {
			snprintf(buf, sizeof(buf), "%s", "psk psk2");
		} else if (strcmp(pvalue, "WPA3-Personal-Transition") == 0) {
			snprintf(buf, sizeof(buf), "%s", "psk2 sae");
		} else if (strcmp(pvalue, "WPA-Enterprise") == 0) {
			snprintf(buf, sizeof(buf), "%s", "wpa");
		} else if (strcmp(pvalue, "WPA2-Enterprise") == 0) {
			snprintf(buf, sizeof(buf), "%s", "wpa2");
		} else if (strcmp(pvalue, "WPA3-Enterprise") == 0) {
			snprintf(buf, sizeof(buf), "%s", "wpa2");
		} else if (strcmp(pvalue, "WPA-WPA2-Enterprise") == 0) {
			snprintf(buf, sizeof(buf), "%s", "wpa wpa2");
		}
		WIFI_DBG("%s: wlcsm_nvram_set %s=%s\n", __FUNCTION__, nvram_name, buf);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	/* CMD_SET_IOCTL is not needed for hostapd, leave the code as is
	 * for lagecy chips, w/o supporting WPA3 */
	if (cmd & CMD_SET_IOCTL) {
		WIFI_DBG("%s: CMD_SET_IOCTL %s!\n", __FUNCTION__ , pvalue);

		/*set both wpa_auth and wsec*/
		if (strncmp(pvalue, "WPA-Personal", strlen("WPA-Personal")) == 0) {
			if (wl_iovar_setint(osifname, "wpa_auth", 0x04)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 68)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if (strncmp(pvalue, "WPA2-Personal", strlen("WPA2-Personal")) == 0) {
			if (wl_iovar_setint(osifname, "wpa_auth", 0x80)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 68)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if (strncmp(pvalue, "WPA-WPA2-Personal", strlen("WPA-WPA2-Personal")) == 0) {

			if (wl_iovar_setint(osifname, "wpa_auth", 0x84)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 70)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}

		} else if (strncmp(pvalue, "WPA2-Enterprise", strlen("WPA2-Enterprise")) == 0) {
			if (wl_iovar_setint(osifname, "wpa_auth", 0x40)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 70)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if (strncmp(pvalue, "WPA-WPA2-Enterprise", strlen("WPA-WPA2-Enterprise")) == 0) {
			if (wl_iovar_setint(osifname, "wpa_auth", 0x42)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 70)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if  (strncmp(pvalue, "None", strlen("None")) == 0) {

			if (wl_iovar_setint(osifname, "wpa_auth", 0x00)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 0)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		} else if  (strncmp(pvalue, "WEP-128", strlen("WEP-128")) == 0) {
			if (wl_iovar_setint(osifname, "wpa_auth", 0x01)) {
				WIFI_ERR("%s: wl_iovar_setint wpa_auth failed !\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_setint(osifname, "wsec", 1)) {
				WIFI_ERR("%s: wl_iovar_setint wsec failed !\n", __FUNCTION__);
				return -1;
			}
		}
	}

	return 0;
}

int
wldm_AccessPoint_Wpa_Encryptionmode(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "WpaEncryptionmode";
	char *osifname, *nvifname, nvram_name[NVRAM_NAME_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue!\n", __FUNCTION__);
		return -1;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *crypto = NULL;
		const char *EncModeStr;

		if (!plen) {
			WIFI_ERR("%s: invalid null plen!\n", __FUNCTION__);
			return -1;
		}

		snprintf(nvram_name, sizeof(nvram_name), "%s_crypto", nvifname);
		crypto = wlcsm_nvram_get(nvram_name);
		if (!crypto) {
			WIFI_ERR("%s: \"nvram_get %s\" fails", __FUNCTION__, nvram_name);
			return -1;
		}

		if (!strcmp(crypto, "aes"))
			EncModeStr = wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_AES];
		else if (!strcmp(crypto, "tkip"))
			EncModeStr = wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP];
		else if (!strcmp(crypto, "tkip+aes") || !strcmp(crypto, "aes+tkip"))
			EncModeStr = wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP_AES];
		else {
			WIFI_ERR("%s: crypto %s is unexpected\n", __FUNCTION__, crypto);
			return -1;
		}

		if (*plen < strlen(EncModeStr) + 1) {
			WIFI_ERR("%s: *plen is not big enough to hold pvalue\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, EncModeStr);
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_Encryption_MASK);
		if (pObj == NULL)
			return -1;

		if (strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_AES],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_AES])) != 0 &&
			strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP_AES],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP_AES])) != 0 &&
			strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP])) != 0)
			pObj->reject_map |= X_RDK_AccessPoint_Security_Encryption_MASK;
		else {
			if (pObj->Security.Encryption)
				free(pObj->Security.Encryption);
			pObj->Security.Encryption = malloc(strlen(pvalue) + 1);
			if (pObj->Security.Encryption) {
				strcpy(pObj->Security.Encryption, pvalue);
				pObj->apply_map |= X_RDK_AccessPoint_Security_Encryption_MASK;
			} else {
				WIFI_ERR("%s malloc failed\n", __FUNCTION__);
				wldm_rel_Object(pObj, FALSE);
				return -1;
			}
		}
		WIFI_DBG("%s: CMD_SET %s!\n", __FUNCTION__ ,pvalue);
		wldm_rel_Object(pObj, (pObj->apply_map &
			X_RDK_AccessPoint_Security_Encryption_MASK) ? TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {
		if (strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_AES],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_AES])) == 0 ) {
			snprintf(nvram_name, sizeof(nvram_name), "%s_crypto", nvifname);
			if (wlcsm_nvram_set(nvram_name, "aes") != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
					__FUNCTION__, nvram_name, pvalue);
				return -1;
			}
		} else if (strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP])) == 0 ) {
			snprintf(nvram_name, sizeof(nvram_name), "%s_crypto", nvifname);
			if (wlcsm_nvram_set(nvram_name, "tkip") != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
					__FUNCTION__, nvram_name, pvalue);
				return -1;
			}
		} else if (strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP_AES],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP_AES])) == 0) {
			snprintf(nvram_name, sizeof(nvram_name), "%s_crypto", nvifname);
			if (wlcsm_nvram_set(nvram_name, "tkip+aes") != 0) {
				WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
					__FUNCTION__, nvram_name, pvalue);
				return -1;
			}
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		if (strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_AES],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_AES])) == 0) {
			int ret = wl_iovar_setint(osifname, "wsec", AES_ENABLED | SES_OW_ENABLED);

			if (ret < 0) {
				WIFI_ERR("%s: wl_iovar_setint wsec returns %d!\n", __FUNCTION__, ret);
				return -1;
			}
		} else if (strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP])) == 0) {
			int ret = wl_iovar_setint(osifname, "wsec", TKIP_ENABLED | SES_OW_ENABLED);

			if (ret < 0) {
				WIFI_ERR("%s: wl_iovar_setint wsec returns %d!\n", __FUNCTION__, ret);
				return -1;
			}
		} else if (strncmp(pvalue, wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP_AES],
			strlen(wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP_AES])) == 0) {
			int ret = wl_iovar_setint(osifname, "wsec", AES_ENABLED | TKIP_ENABLED | SES_OW_ENABLED);

			if (ret < 0) {
				WIFI_ERR("%s: wl_iovar_setint wsec returns %d!\n", __FUNCTION__, ret);
				return -1;
			}
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_AuthMode(int cmd, int apIndex,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "AuthMode";
	int authmode;
	int ret;
	char *osifname, *nvifname, nvram_name[NVRAM_NAME_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_GET );

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_AuthMode_MASK);
		if (pObj == NULL)
			return -1;

		if (*pvalue > 4 || *pvalue < 0 ) {
			pObj->reject_map |= X_RDK_AccessPoint_Security_AuthMode_MASK;
		} else {
			if (*pvalue > 1) {
				authmode = 2;
			} else {
				authmode = *pvalue;
			}
			pObj->Security.AuthMode = authmode;
			pObj->apply_map |= X_RDK_AccessPoint_Security_AuthMode_MASK;

			WIFI_DBG("%s: CMD_SET %d \n", __FUNCTION__ ,*pvalue);
		}
		wldm_rel_Object(pObj, pObj->apply_map & X_RDK_AccessPoint_Security_AuthMode_MASK ?
			TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char auth_mode_var[16] = {0};

		if (*pvalue == 0) {
			strcpy(auth_mode_var, "0");
		} else if (*pvalue  == 1) {
			strcpy(auth_mode_var, "1");
		} else {
			strcpy(auth_mode_var, "2");
		}

		snprintf(nvram_name, sizeof(nvram_name), "%s_auth", nvifname);
		if (wlcsm_nvram_set(nvram_name, auth_mode_var) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%d failed!\n", __FUNCTION__, nvram_name, *pvalue);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		ret = wl_iovar_setint(osifname, "auth", *pvalue);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_getint auth returns %d!\n", __FUNCTION__, ret);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_MFPConfig(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "MFPConfig";
	char *osifname, *nvifname, nvram_name[NVRAM_NAME_SIZE];
	int ret;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL );

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);

	if (cmd & (CMD_GET)) {
		int mfp_mode=111, mfp_config;

		ret = wl_iovar_getint(osifname, "mfp", &mfp_mode);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_getint mfp returns %d!\n", __FUNCTION__, ret);
			return -1;
		}
		mfp_config = (mfp_mode == 2) ? MFP_CONFIG_REQUIRED :
			(mfp_mode == 1) ? MFP_CONFIG_CAPABLE : MFP_CONFIG_DISABLE;
		snprintf(pvalue, *plen, "%s", mfpModeStr[mfp_config]);
	}

	if (cmd & (CMD_GET_NVRAM)) {
		int mfp_mode, mfp_config;

		snprintf(nvram_name, sizeof(nvram_name), "%s_mfp", nvifname);
		mfp_mode = atoi(nvram_safe_get(nvram_name));
		mfp_config = (mfp_mode == 2) ? MFP_CONFIG_REQUIRED :
			(mfp_mode == 1) ? MFP_CONFIG_CAPABLE : MFP_CONFIG_DISABLE;
		snprintf(pvalue, *plen, "%s", mfpModeStr[mfp_config]);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;
		pObj = wldm_get_AccessPointSecurityObject(apIndex,
				AccessPoint_Security_MFPConfig_MASK);
		if (pObj == NULL)
			return -1;

		if ((strncmp(pvalue, mfpModeStr[MFP_CONFIG_DISABLE],
			strlen(mfpModeStr[MFP_CONFIG_DISABLE])) != 0) &&
			(strncmp(pvalue, mfpModeStr[MFP_CONFIG_CAPABLE],
			strlen(mfpModeStr[MFP_CONFIG_CAPABLE])) != 0) &&
			(strncmp(pvalue, mfpModeStr[MFP_CONFIG_REQUIRED],
			strlen(mfpModeStr[MFP_CONFIG_REQUIRED])) != 0)) {
			pObj->reject_map |= AccessPoint_Security_MFPConfig_MASK;
		} else {
			if (pObj->Security.MFPConfig)
				free(pObj->Security.MFPConfig);

			pObj->Security.MFPConfig = malloc(strlen(pvalue) + 1);
			if (pObj->Security.MFPConfig) {
				strcpy(pObj->Security.MFPConfig, pvalue);
				pObj->apply_map |= AccessPoint_Security_MFPConfig_MASK;
			} else {
				WIFI_ERR("%s malloc failed\n", __FUNCTION__);
				wldm_rel_Object(pObj, FALSE);
				return -1;
			}
			WIFI_DBG("%s: CMD_SET %s \n", __FUNCTION__ ,pvalue);
		}
		wldm_rel_Object(pObj, (pObj->apply_map & AccessPoint_Security_MFPConfig_MASK) ?
			TRUE : FALSE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char mfp_mode_var[16] = {0};

		if (strncmp(pvalue, mfpModeStr[MFP_CONFIG_DISABLE],
			strlen(mfpModeStr[MFP_CONFIG_DISABLE])) == 0) {
			strcpy(mfp_mode_var, "0");
		} else if (strncmp(pvalue, mfpModeStr[MFP_CONFIG_CAPABLE],
			strlen(mfpModeStr[MFP_CONFIG_CAPABLE])) == 0) {
                        strcpy(mfp_mode_var, "1");
		} else if (strncmp(pvalue, mfpModeStr[MFP_CONFIG_REQUIRED],
			strlen(mfpModeStr[MFP_CONFIG_REQUIRED])) == 0) {
			strcpy(mfp_mode_var, "2");
		}

		snprintf(nvram_name, sizeof(nvram_name), "%s_mfp", nvifname);
		if (wlcsm_nvram_set(nvram_name, mfp_mode_var) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, mfp_mode_var);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		int mfp_mode = 0;
		if (strncmp(pvalue, mfpModeStr[MFP_CONFIG_DISABLE],
			strlen(mfpModeStr[MFP_CONFIG_DISABLE])) == 0) {
			mfp_mode = MFP_CONFIG_DISABLE;
		} else if (strncmp(pvalue, mfpModeStr[MFP_CONFIG_CAPABLE],
			strlen(mfpModeStr[MFP_CONFIG_CAPABLE])) == 0) {
			mfp_mode = MFP_CONFIG_CAPABLE;
		} else if (strncmp(pvalue, mfpModeStr[MFP_CONFIG_REQUIRED],
			strlen(mfpModeStr[MFP_CONFIG_REQUIRED])) == 0) {
			mfp_mode = MFP_CONFIG_REQUIRED;
		}

		ret = wl_iovar_setint(osifname, "mfp", mfp_mode);
		if (ret < 0) {
			WIFI_ERR("%s: wl_iovar_getint mfp returns %d!\n", __FUNCTION__, ret);
			return -1;
		}
	}
	return 0;
}

int
wldm_AccessPoint_Security_Modessupported(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ModesSupported";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		int size = 0, len = *plen, i, num;

		num = sizeof(wldm_supported_security_modes)/sizeof(wldm_supported_security_modes[0]);
		for (i = 0; i < num; ++i) {
			if (size == 0)
				size = snprintf(pvalue, len - size, "%s", wldm_supported_security_modes[i]);
			else
				size += snprintf(pvalue + size, len - size, ", %s", wldm_supported_security_modes[i]);
		}
	}

	return 0;
}

/* nvram variable 'wpa_psk' is used to store psk or passphase,
 * use the length to diffrentiate psk or passphase,
 * psk is stored in the format of 64 HEX digits, length is 64,
 * passphrase is a string that has 8..63 characters.
 */
int
wldm_AccessPoint_Security_PreSharedKey(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "PreSharedKey";
	char *nvifname, nvram_name[NVRAM_NAME_SIZE];
	int len;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	nvifname = wldm_get_nvifname(apIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *psk;

		if (!plen) {
			WIFI_ERR("%s: invalid null plen\n", __FUNCTION__);
			return -1;
		}

		snprintf(nvram_name, sizeof(nvram_name), "%s_wpa_psk", nvifname);
		psk = nvram_safe_get(nvram_name);
		len = strlen(psk);
		/* RDK wrongly calls wifi_setApSecurityPreSharedKey to set WPA passphrase,
		 * remove the validation of psk temporary to workaround, until RDK fix it */
		if (*plen <= len) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, psk);

		if (cmd & CMD_LIST) {
			if (cmd & CMD_GET)
				PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, "xxx"); /* Hide */
			else
				PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", nvram_name, pvalue);
		}
	}

	if (cmd == CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex,
				AccessPoint_Security_PreSharedKey_MASK);
		if (pObj == NULL)
			return -1;

		/* RDK wrongly calls wifi_setApSecurityPreSharedKey to set WPA passphrase,
		 * remove the validation of psk temporary to workaround, until RDK fix it */
		if (pObj->Security.PreSharedKey)
			free(pObj->Security.PreSharedKey);
		pObj->Security.PreSharedKey = strdup(pvalue);
		if (!pObj->Security.PreSharedKey) {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		pObj->apply_map |= AccessPoint_Security_PreSharedKey_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		/* RDK wrongly calls wifi_setApSecurityPreSharedKey to set WPA passphrase,
		 * remove the validation of psk temporary to workaround, until RDK fix it */
		snprintf(nvram_name, sizeof(nvram_name), "%s_wpa_psk", nvifname);
		if (wlcsm_nvram_set(nvram_name, pvalue) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, nvram_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusServerIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RadiusServerIPAddr";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *ip_addr = NULL, *default_ip_addr = "0.0.0.0";
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		if (!plen) {
			WIFI_ERR("%s: invalid null plen\n", __FUNCTION__);
			return -1;
		}

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_ipaddr", nvifname);

		ip_addr = nvram_safe_get(nv_name);
		if (*ip_addr == '\0') {
			/* return the default value same as default in hostapd */
			WIFI_DBG("%s: %d: return default ip addr %s\n",
				__FUNCTION__, apIndex, default_ip_addr);
			ip_addr = default_ip_addr;
		}

		if (*plen < strlen(ip_addr) + 1) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, ip_addr);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex,
			AccessPoint_Security_RadiusServerIPAddr_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Security.RadiusServerIPAddr) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Security.RadiusServerIPAddr);
			free(pObj->Security.RadiusServerIPAddr);
		}
		pObj->Security.RadiusServerIPAddr = strdup(pvalue);
		if (pObj->Security.RadiusServerIPAddr) {
			pObj->apply_map |= AccessPoint_Security_RadiusServerIPAddr_MASK;
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_ipaddr", nvifname);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, pvalue);
		if (wlcsm_nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusServerPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RadiusServerPort";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *pPort = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];
		unsignedInt default_port = 1812;

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_port", nvifname);

		pPort = nvram_safe_get(nv_name);
		*pvalue = atoi(pPort);
		if (*pvalue == 0) {
			/* return the default value same as default in hostapd */
			WIFI_DBG("%s: %d: return default port %u\n",
				__FUNCTION__, apIndex, default_port);
			*pvalue = default_port;
		}
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex,
			AccessPoint_Security_RadiusServerPort_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Security.RadiusServerPort = *pvalue;
		pObj->apply_map |= AccessPoint_Security_RadiusServerPort_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE], nv_value[BUF_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_port", nvifname);
		snprintf(nv_value, sizeof(nv_value), "%d", *pvalue);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusSecret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "RadiusSecret";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *secret = NULL, *default_secret = "12345678";
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		if (!plen) {
			WIFI_ERR("%s: invalid null plen\n", __FUNCTION__);
			return -1;
		}

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_key", nvifname);

		secret = nvram_safe_get(nv_name);
		if (*secret == '\0') {
			/* return the default value same as default in hostapd */
			WIFI_DBG("%s: %d: return default secret %s\n",
				__FUNCTION__, apIndex, default_secret);
			secret = default_secret;
		}

		if (*plen < strlen(secret) + 1) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, secret);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex,
			AccessPoint_Security_RadiusSecret_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Security.RadiusSecret) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Security.RadiusSecret);
			free(pObj->Security.RadiusSecret);
		}
		pObj->Security.RadiusSecret = strdup(pvalue);
		if (pObj->Security.RadiusSecret) {
			pObj->apply_map |= AccessPoint_Security_RadiusSecret_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_key", nvifname);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, pvalue);
		if (wlcsm_nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_SecondaryRadiusServerIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SecondaryRadiusServerIPAddr";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char* ip_addr = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		if (!plen) {
			WIFI_ERR("%s: invalid null plen\n", __FUNCTION__);
			return -1;
		}

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius2_ipaddr", nvifname);

		ip_addr = wlcsm_nvram_get(nv_name);
		if (!ip_addr) {
			WIFI_DBG("%s: secondary RADIUS ip not set yet!\n", __FUNCTION__);
			return -1;
		}

		if (*plen < strlen(ip_addr) + 1) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, ip_addr);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex,
			AccessPoint_Security_SecondaryRadiusServerIPAddr_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Security.SecondaryRadiusServerIPAddr) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Security.SecondaryRadiusServerIPAddr);
			free(pObj->Security.SecondaryRadiusServerIPAddr);
		}
		pObj->Security.SecondaryRadiusServerIPAddr = strdup(pvalue);
		if (pObj->Security.SecondaryRadiusServerIPAddr) {
			pObj->apply_map |= AccessPoint_Security_SecondaryRadiusServerIPAddr_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius2_ipaddr", nvifname);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, pvalue);
		if (wlcsm_nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_SecondaryRadiusServerPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SecondaryRadiusServerPort";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *pPort = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];
		unsignedInt default_port = 1812;

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius2_port", nvifname);

		pPort = nvram_safe_get(nv_name);
		*pvalue = atoi(pPort);
		if (*pvalue == 0) {
			/* return the default value same as default in hostapd */
			WIFI_DBG("%s: %d: return default port %u\n",
				__FUNCTION__, apIndex, default_port);
			*pvalue = default_port;
		}
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex,
			AccessPoint_Security_SecondaryRadiusServerPort_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Security.SecondaryRadiusServerPort = *pvalue;
		pObj->apply_map |= AccessPoint_Security_SecondaryRadiusServerPort_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE], nv_value[BUF_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius2_port", nvifname);
		snprintf(nv_value, sizeof(nv_value), "%d", *pvalue);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_SecondaryRadiusSecret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SecondaryRadiusSecret";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *secret = NULL, *default_secret = "12345678";
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		if (!plen) {
			WIFI_ERR("%s: invalid null plen\n", __FUNCTION__);
			return -1;
		}

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius2_key", nvifname);

		secret = nvram_safe_get(nv_name);
		if (*secret == '\0') {
			/* return the default value same as default in hostapd */
			WIFI_DBG("%s: %d: return default secret %s\n",
				__FUNCTION__, apIndex, default_secret);
			secret = default_secret;
		}

		if (*plen < strlen(secret) + 1) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, secret);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_Object *pObj;

		pObj = wldm_get_AccessPointSecurityObject(apIndex,
			AccessPoint_Security_SecondaryRadiusSecret_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Security.SecondaryRadiusSecret) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Security.SecondaryRadiusSecret);
			free(pObj->Security.SecondaryRadiusSecret);
		}
		pObj->Security.SecondaryRadiusSecret = strdup(pvalue);
		if (pObj->Security.SecondaryRadiusSecret) {
			pObj->apply_map |= AccessPoint_Security_SecondaryRadiusSecret_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius2_key", nvifname);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, pvalue);
		if (wlcsm_nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusReAuthInterval(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "RadiusReAuthInterval";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE], nv_value[512];
	char *p_nv_value;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue || !plen) {
		WIFI_ERR("%s: invalid null pvalue or plen\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		nv_ifname = wldm_get_nvifname(apIndex);

		snprintf(nv_name, sizeof(nv_name), "%s_radius_reauth_interval", nv_ifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		if (!p_nv_value)
			*pvalue = 3600; /* default value is 3600 in hostapd */
		else
			*pvalue = strtoul(p_nv_value, NULL, 10);
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_RadiusReAuthInterval_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Security.RadiusReAuthInterval = *pvalue;
		pObj->apply_map |= X_RDK_AccessPoint_Security_RadiusReAuthInterval_MASK;

		WIFI_DBG("%s: CMD_SET %u\n", __FUNCTION__, *pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_reauth_interval", nv_ifname);
		snprintf(nv_value, sizeof(nv_value), "%u", *pvalue);

		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%u failed!\n", __FUNCTION__, nv_name, *pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusOperatorName(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "RadiusOperatorName";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE], nv_value[512];
	char *p_nv_value, *pCh;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue || !plen) {
		WIFI_ERR("%s: invalid null pvalue or plen\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		nv_ifname = wldm_get_nvifname(apIndex);

		snprintf(nv_name, sizeof(nv_name), "%s_radius_operator_name", nv_ifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		if (!p_nv_value) {
			WIFI_ERR("%s: operator name not set yet\n", __FUNCTION__);
			return -1;
		}

		/* value format: <radius_attr_id>[:<syntax:value>] */
		pCh = strchr(p_nv_value, ':'); /* skip attr_id */
		if (pCh) {
			++pCh;
			pCh = strchr(pCh, ':'); /* skip syntax */
			if (pCh) {
				++pCh; /* pCh now points to real value */

				snprintf(pvalue, *plen, pCh);
				return 0;
			}
		}

		WIFI_ERR("%s: format of stored operator name is wrong\n", __FUNCTION__);
		return -1;
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_RadiusOperatorName_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Security.RadiusOperatorName) {
			WIFI_ERR("%s: free old RadiusOperatorName\n", __FUNCTION__);
			free(pObj->Security.RadiusOperatorName);
			pObj->Security.RadiusOperatorName = NULL;
		}
		pObj->Security.RadiusOperatorName = strdup(pvalue);
		if (!pObj->Security.RadiusOperatorName) {
			WIFI_ERR("%s: malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		pObj->apply_map |= X_RDK_AccessPoint_Security_RadiusOperatorName_MASK;

		WIFI_DBG("%s: CMD_SET %s\n", __FUNCTION__, pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_operator_name", nv_ifname);
		/* value format: <radius_attr_id>[:<syntax:value>]
		 *
		 * Operator-Name attribute id is 126
		 * syntax is 's', means utf-8 string
		 */
		snprintf(nv_value, sizeof(nv_value), "126:s:%s", pvalue);

		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusLocationData(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "RadiusLocationData";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE], nv_value[512];
	char *p_nv_value, *pCh;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue || !plen) {
		WIFI_ERR("%s: invalid null pvalue or plen\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		nv_ifname = wldm_get_nvifname(apIndex);

		snprintf(nv_name, sizeof(nv_name), "%s_radius_location_data", nv_ifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		if (!p_nv_value) {
			WIFI_ERR("%s: location data not set yet\n", __FUNCTION__);
			return -1;
		}

		/* value format: <radius_attr_id>[:<syntax:value>] */
		pCh = strchr(p_nv_value, ':'); /* skip attr_id */
		if (pCh) {
			++pCh;
			pCh = strchr(pCh, ':'); /* skip syntax */
			if (pCh) {
				++pCh; /* pCh now points to real value */

				snprintf(pvalue, *plen, pCh);
				return 0;
			}
		}

		WIFI_ERR("%s: format of stored location data is wrong\n", __FUNCTION__);
		return -1;
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_RadiusLocationData_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Security.RadiusLocationData) {
			WIFI_ERR("%s: free old RadiusLocationData\n", __FUNCTION__);
			free(pObj->Security.RadiusLocationData);
			pObj->Security.RadiusLocationData = NULL;
		}
		pObj->Security.RadiusLocationData = strdup(pvalue);
		if (!pObj->Security.RadiusLocationData) {
			WIFI_ERR("%s: malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		pObj->apply_map |= X_RDK_AccessPoint_Security_RadiusLocationData_MASK;

		WIFI_DBG("%s: CMD_SET %s\n", __FUNCTION__, pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_location_data", nv_ifname);
		/* value format: <radius_attr_id>[:<syntax:value>]
		 *
		 * Locata-Data attribute id is 128
		 * syntax is 's', means utf-8 string
		 */
		snprintf(nv_value, sizeof(nv_value), "128:s:%s", pvalue);

		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusDASPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "RadiusDASPort";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE], nv_value[512];
	char *p_nv_value;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		nv_ifname = wldm_get_nvifname(apIndex);

		snprintf(nv_name, sizeof(nv_name), "%s_radius_das_port", nv_ifname);
		p_nv_value = nvram_get(nv_name);
		if (!p_nv_value) {
			WIFI_ERR("%s: %d: das port not set yet!\n", __FUNCTION__, apIndex);
			return -1;
		}
		*pvalue = atoi(p_nv_value);
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_RadiusDASPort_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Security.RadiusDASPort = *pvalue;
		pObj->apply_map |= X_RDK_AccessPoint_Security_RadiusDASPort_MASK;

		WIFI_DBG("%s: %d: CMD_SET %d\n", __FUNCTION__, apIndex, *pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_das_port", nv_ifname);
		snprintf(nv_value, sizeof(nv_value), "%d", *pvalue);

		if (nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s: %d: nvram_set %s=%s failed!\n",
				__FUNCTION__, apIndex, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusDASClientIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "RadiusDASClientIPAddr";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE];
	char *p_nv_value;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		if (!plen) {
			WIFI_ERR("%s: %d: invalid null plen\n", __FUNCTION__, apIndex);
			return -1;
		}

		nv_ifname = wldm_get_nvifname(apIndex);

		snprintf(nv_name, sizeof(nv_name), "%s_radius_das_client_ipaddr", nv_ifname);
		p_nv_value = nvram_get(nv_name);
		if (!p_nv_value) {
			WIFI_ERR("%s: %d: das client ip not set yet!\n", __FUNCTION__, apIndex);
			return -1;
		}
		snprintf(pvalue, *plen, "%s", p_nv_value);
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_RadiusDASClientIPAddr_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Security.RadiusDASClientIPAddr) {
			free(pObj->Security.RadiusDASClientIPAddr);
			pObj->Security.RadiusDASClientIPAddr = NULL;
		}

		pObj->Security.RadiusDASClientIPAddr = strdup(pvalue);
		if (pObj->Security.RadiusDASClientIPAddr == NULL) {
			WIFI_ERR("%s: %d: fail to allocate memory for CMD_SET\n", __FUNCTION__, apIndex);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		pObj->apply_map |= X_RDK_AccessPoint_Security_RadiusDASClientIPAddr_MASK;

		WIFI_DBG("%s: %d: CMD_SET %s\n", __FUNCTION__, apIndex, pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_das_client_ipaddr", nv_ifname);

		if (nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s: %d: nvram_set %s=%s failed!\n",
				__FUNCTION__, apIndex, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusDASSecret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "RadiusDASSecret";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE];
	char *p_nv_value;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		if (!plen) {
			WIFI_ERR("%s: %d: invalid null plen\n", __FUNCTION__, apIndex);
			return -1;
		}

		nv_ifname = wldm_get_nvifname(apIndex);

		snprintf(nv_name, sizeof(nv_name), "%s_radius_das_key", nv_ifname);
		p_nv_value = nvram_get(nv_name);
		if (!p_nv_value) {
			WIFI_ERR("%s: %d: das secret not set yet!\n", __FUNCTION__, apIndex);
			return -1;
		}
		snprintf(pvalue, *plen, "%s", p_nv_value);
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_RadiusDASSecret_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Security.RadiusDASSecret) {
			free(pObj->Security.RadiusDASSecret);
			pObj->Security.RadiusDASSecret = NULL;
		}

		pObj->Security.RadiusDASSecret = strdup(pvalue);
		if (pObj->Security.RadiusDASSecret == NULL) {
			WIFI_ERR("%s: %d: fail to allocate memory for CMD_SET\n", __FUNCTION__, apIndex);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		pObj->apply_map |= X_RDK_AccessPoint_Security_RadiusDASSecret_MASK;

		WIFI_DBG("%s: %d: CMD_SET %s\n", __FUNCTION__, apIndex, pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_das_key", nv_ifname);

		if (nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s: %d: nvram_set %s=%s failed!\n",
				__FUNCTION__, apIndex, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_RadiusGreylist(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "RadiusGreylist";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE], nv_value[512];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		nv_ifname = wldm_get_nvifname(apIndex);

		snprintf(nv_name, sizeof(nv_name), "%s_rdk_radius_greylist", nv_ifname);
		*pvalue = nvram_match(nv_name, "1");
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_RadiusGreylist_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Security.RadiusGreylistEnable = *pvalue;
		pObj->apply_map |= X_RDK_AccessPoint_Security_RadiusGreylist_MASK;

		WIFI_DBG("%s: %d: CMD_SET %d\n", __FUNCTION__, apIndex, *pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_rdk_radius_greylist", nv_ifname);
		snprintf(nv_value, sizeof(nv_value), "%d", *pvalue);

		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s: %d: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, apIndex, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_WPAPairwiseRetries(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{

	char *parameter = "WPAPairwiseRetries";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE], nv_value[64];
	char *p_nv_value;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_wpa_pairwise_retries", nv_ifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		*pvalue = p_nv_value ? atoi(p_nv_value) : 4; /* default value in hostapd is 4 */
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_WPAPairwiseRetries_MASK);
		if (pObj == NULL)
			return -1;

		if (*pvalue < 1) {
			WIFI_DBG("%s: %d: invalid value [%u], allowed range 1..4294967295\n",
				__FUNCTION__, apIndex, *pvalue);
			pObj->reject_map |= X_RDK_AccessPoint_Security_WPAPairwiseRetries_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		pObj->Security.WPAPairwiseRetries = *pvalue;
		pObj->apply_map |= X_RDK_AccessPoint_Security_WPAPairwiseRetries_MASK;

		WIFI_DBG("%s: %d: CMD_SET %u\n", __FUNCTION__, *pvalue, apIndex);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		if (*pvalue < 1) {
			WIFI_DBG("%s: %d: invalid value [%u], allowed range 1..4294967295\n",
				__FUNCTION__, apIndex, *pvalue);
			return -1;
		}

		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_wpa_pairwise_retries", nv_ifname);
		snprintf(nv_value, sizeof(nv_value), "%u", *pvalue);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s: %d: wlcsm_nvram_set %s=%u failed!\n", __FUNCTION__,
				apIndex, nv_name, *pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Security_EncryptionModesSupported(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "EncryptionModesSupported";
	char encModesStr[256] = {0};

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL | CMD_GET_NVRAM);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if ((!pvalue) || (!plen)) {
		WIFI_ERR("%s: %d: Error null ptr\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (cmd & CMD_GET) {
		snprintf(encModesStr, sizeof(encModesStr), "%s,%s",
			wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_TKIP],
			wpaEncryptionModeStr[WPA_ENCRYPTION_MODE_AES]);
		if (*plen < (strlen(encModesStr) + 1)) {
			WIFI_ERR("%s: len too short! need %d got %d\n",
				__FUNCTION__, (strlen(encModesStr)+1), *plen);
			return -1;
		}
		*plen = strlen(encModesStr) + 1;
		strncpy(pvalue, encModesStr, *plen);
	}
	return 0;
}

int
wldm_AccessPoint_Security_WPAPMKLifetime(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "WPAPMKLifetime";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE], nv_value[512];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *p_nv_value;

		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_wpa_pmk_lifetime", nv_ifname);
		p_nv_value = nvram_get(nv_name);
		*pvalue = p_nv_value ? atoi(p_nv_value) : 43200; /* default value is 12 hours in hostapd */
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_WPAPMKLifetime_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Security.WPAPMKLifetime = *pvalue;
		pObj->apply_map |= X_RDK_AccessPoint_Security_WPAPMKLifetime_MASK;

		WIFI_DBG("%s: %d: CMD_SET %d\n", __FUNCTION__, apIndex, *pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_wpa_pmk_lifetime", nv_ifname);
		snprintf(nv_value, sizeof(nv_value), "%d", *pvalue);

		if (nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s: %d: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, apIndex, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

/*
 * Bitmap bits of nvram 'transition_disable':
 * bit 0 (0x01): WPA3-Personal (i.e., disable WPA2-Personal = WPA-PSK and only
 *	allow SAE to be used)
 * bit 1 (0x02): SAE-PK (disable SAE without use of SAE-PK)
 * bit 2 (0x04): WPA3-Enterprise (move to requiring PMF)
 * bit 3 (0x08): Enhanced Open (disable use of open network; require OWE)
 */
int
wldm_AccessPoint_Security_WPA3TransitionDisable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "WPA3TransitionDisable";
	char *nv_ifname, nv_name[NVRAM_NAME_SIZE], nv_value[512];
	char *p_nv_value;
	unsigned long transition_disable;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_transition_disable", nv_ifname);
		p_nv_value = nvram_safe_get(nv_name);
		transition_disable = strtoul(p_nv_value, NULL, 0);
		*pvalue = (transition_disable & (1ul << 0));
	}

	if (cmd & CMD_SET) {
		X_RDK_AccessPoint_Security_Object *pObj;

		pObj = wldm_get_X_RDK_AccessPointSecurityObject(apIndex,
				X_RDK_AccessPoint_Security_WPA3TransitionDisable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Security.WPA3TransitionDisable = *pvalue;
		pObj->apply_map |= X_RDK_AccessPoint_Security_WPA3TransitionDisable_MASK;

		WIFI_DBG("%s: %d: CMD_SET %d\n", __FUNCTION__, apIndex, *pvalue);
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		nv_ifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_transition_disable", nv_ifname);

		p_nv_value = nvram_safe_get(nv_name);
		transition_disable = strtoul(p_nv_value, NULL, 0);

		if (*pvalue)
			transition_disable |= (1ul << 0);
		else
			transition_disable &= ~(1ul << 0);
		snprintf(nv_value, sizeof(nv_value), "0x%lx", transition_disable);

		if (nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s: %d: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, apIndex, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

/*********************************************************************
*  Device.WiFi.AccessPoint.{i}.Security.X_COMCAST_COM_RadiusSettings.
**********************************************************************/
int
wldm_AccessPoint_Security_X_COMCAST_COM_RadiusSettings(int cmd, int apIndex,
	void *pvalue, int *plen, char *pbuf, int *pbufsz)
{
#define RADIUS_DIS_WPA2_PMK_CACHE 0x00200000
	char *parameter = "X_COMCAST_COM_RadiusSettings";
	Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings *pRadiusSettings = NULL;
	char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE], *p_nv_value;

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		if (*plen != sizeof(Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings)) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}

		pRadiusSettings = (Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings *)pvalue;

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_max_retransmit", nvifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		if (!p_nv_value)
			return -1;
		pRadiusSettings->RadiusServerRetries = atoi(p_nv_value);

		snprintf(nv_name, sizeof(nv_name), "%s_radius_fixed_interval", nvifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		if (!p_nv_value)
			return -1;
		pRadiusSettings->RadiusServerRequestTimeout = atoi(p_nv_value);

		p_nv_value = wlcsm_nvram_get("radius_wpa_pmk_lifetime");
		if (!p_nv_value)
			return -1;
		pRadiusSettings->PMKLifetime = atoi(p_nv_value);

		snprintf(nv_name, sizeof(nv_name), "%s_radius_flags", nvifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		if (!p_nv_value)
			return -1;
		pRadiusSettings->PMKCaching  = (strtoul(p_nv_value, NULL, 0) & RADIUS_DIS_WPA2_PMK_CACHE) ? FALSE : TRUE;

		snprintf(nv_name, sizeof(nv_name), "%s_net_reauth", nvifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		if (!p_nv_value)
			return -1;
		pRadiusSettings->PMKCacheInterval = atoi(p_nv_value);

		p_nv_value = wlcsm_nvram_get("radius_max_auth_attempts");
		if (!p_nv_value)
			return -1;
		pRadiusSettings->MaxAuthenticationAttempts = atoi(p_nv_value);

		p_nv_value = wlcsm_nvram_get("radius_black_timeout");
		if (!p_nv_value)
			return -1;
		pRadiusSettings->BlacklistTableTimeout = atoi(p_nv_value);

		p_nv_value = wlcsm_nvram_get("radius_identity_interval");
		if (!p_nv_value)
			return -1;
		pRadiusSettings->IdentityRequestRetryInterval = atoi(p_nv_value);

		p_nv_value = wlcsm_nvram_get("radius_quiet_period");
		if (!p_nv_value)
			return -1;
		pRadiusSettings->QuietPeriodAfterFailedAuthentication = atoi(p_nv_value);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Security_X_COMCAST_COM_RadiusSettings_Object *pObj;

		if (*plen != sizeof(Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings)) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}

		pObj = wldm_get_AccessPointSecurity_X_COMCAST_COM_RadiusSettingsObject(apIndex);
		if (pObj == NULL)
			return -1;

		memcpy((void *)&pObj->RadiusSettings, pvalue, sizeof(pObj->RadiusSettings));
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nv_value[BUF_SIZE], *p_nv_name;
		unsignedInt radius_flag = 0;

		if (*plen != sizeof(Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings)) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}

		pRadiusSettings = (Device_WiFi_AccessPoint_Security_X_COMCAST_COM_RadiusSettings *)pvalue;

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_max_retransmit", nvifname);
		snprintf(nv_value, sizeof(nv_value), "%d", pRadiusSettings->RadiusServerRetries);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}

		snprintf(nv_name, sizeof(nv_name), "%s_radius_fixed_interval", nvifname);
		snprintf(nv_value, sizeof(nv_value), "%d", pRadiusSettings->RadiusServerRequestTimeout);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}

		p_nv_name = "radius_wpa_pmk_lifetime";
		snprintf(nv_value, sizeof(nv_value), "%d", pRadiusSettings->PMKLifetime);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, p_nv_name, nv_value);
		if (wlcsm_nvram_set(p_nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, p_nv_name, nv_value);
			return -1;
		}

		snprintf(nv_name, sizeof(nv_name), "%s_radius_flags", nvifname);
		p_nv_value = wlcsm_nvram_get(nv_name);
		if (p_nv_value)
			radius_flag = strtoul(p_nv_value, NULL, 0);
		if (pRadiusSettings->PMKCaching == TRUE)
			radius_flag &= ~RADIUS_DIS_WPA2_PMK_CACHE;
		else
			radius_flag |= RADIUS_DIS_WPA2_PMK_CACHE;

		snprintf(nv_value, sizeof(nv_value), "0x%x", radius_flag);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}

		snprintf(nv_name, sizeof(nv_name), "%s_net_reauth", nvifname);
		snprintf(nv_value, sizeof(nv_value), "%d", pRadiusSettings->PMKCacheInterval);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}

		p_nv_name = "radius_max_auth_attempts";
		snprintf(nv_value, sizeof(nv_value), "%d", pRadiusSettings->MaxAuthenticationAttempts);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, p_nv_name, nv_value);
		if (wlcsm_nvram_set(p_nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, p_nv_name, nv_value);
			return -1;
		}

		p_nv_name = "radius_black_timeout";
		snprintf(nv_value, sizeof(nv_value), "%d", pRadiusSettings->BlacklistTableTimeout);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, p_nv_name, nv_value);
		if (wlcsm_nvram_set(p_nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, p_nv_name, nv_value);
			return -1;
		}

		p_nv_name = "radius_identity_interval";
		snprintf(nv_value, sizeof(nv_value), "%d", pRadiusSettings->IdentityRequestRetryInterval);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, p_nv_name, nv_value);
		if (wlcsm_nvram_set(p_nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, p_nv_name, nv_value);
			return -1;
		}

		p_nv_name = "radius_quiet_period";
		snprintf(nv_value, sizeof(nv_value), "%d", pRadiusSettings->QuietPeriodAfterFailedAuthentication);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, p_nv_name, nv_value);
		if (wlcsm_nvram_set(p_nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, p_nv_name, nv_value);
			return -1;
		}

	}

	return 0;
}

/**********************************
*  Device.WiFi.AccessPoint.{i}.WPS.
**********************************/
wldm_enum_to_str_map_t wldm_wps_supported_config_methods_table[] = {
	{WPS_CONFIG_LABEL,		"Label"},
	{WPS_CONFIG_KEYPAD,		"Keypad"},
	{WPS_CONFIG_DISPLAY,		"Display"},
	{WPS_CONFIG_VIRT_DISPLAY,	"VirtualDisplay"},
	{WPS_CONFIG_PHY_DISPLAY,	"PhysicalDisplay"},
	{WPS_CONFIG_PUSHBUTTON,		"PushButton"},
	{WPS_CONFIG_VIRT_PUSHBUTTON,	"VirtualPushButton"},
	{WPS_CONFIG_PHY_PUSHBUTTON,	"PhysicalPushButton"},
#ifdef NOTYET
	{WPS_CONFIG_USBA,		"USBFlashDrive"},
	{WPS_CONFIG_ETHERNET,		"Ethernet"},
	{WPS_CONFIG_EXT_NFC_TOKEN,	"ExternalNFCToken"},
	{WPS_CONFIG_INT_NFC_TOKEN,	"IntegratedNFCToken"},
	{WPS_CONFIG_NFC_INTERFACE,	"NFCInterface"},
#endif /* NOTYET */
	{0xffff,			NULL}
};

/********************************************************
*  Device.WiFi.AccessPoint.{i}.WPS.ConfigMethodsSupported
********************************************************/
int
wldm_AccessPoint_WPS_ConfigMethodsSupported(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ConfigMethodsSupported";

	IGNORE_CMD_WARNING(cmd,
		CMD_SET | CMD_ADD | CMD_DEL | CMD_GET_NVRAM | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		int write_size = 0, remain_size, ret, i = 0;
		const char *method;

		if (!pvalue) {
			WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
			return -1;
		}

		remain_size = *plen;
		while (TRUE) {
			method = wldm_wps_supported_config_methods_table[i].str_val;
			if (method == NULL)
				break;

			ret = snprintf(pvalue + write_size, remain_size,
					(write_size == 0) ? "%s" : ",%s", method);
			if (ret < 0) {
				WIFI_ERR("%s: snprintf failed with return value [%d]\n",
					__FUNCTION__, ret);
				return -1;
			} else if (ret >= remain_size) {
				WIFI_ERR("%s: output is truncated\n", __FUNCTION__);
				return -1;
			}
			write_size += ret;
			remain_size -= ret;
			++i;
		}

	}

	return 0;
}

/********************************************************
*  Device.WiFi.AccessPoint.{i}.WPS.ConfigMethodsEnabled
********************************************************/
int
wldm_AccessPoint_WPS_ConfigMethodsEnabled(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ConfigMethodsEnabled";
	unsigned long iwpsmethods, iwpsmethod;
	const char *method_str;
	int i;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *nvram_value = NULL;
		int ret, write_size, remain_size;

		if (!plen) {
			WIFI_ERR("%s: invalid null plen\n", __FUNCTION__);
			return -1;
		}
		nvram_value = wlcsm_nvram_get("wps_config_method");
		if (nvram_value && nvram_value[0] != '\0')
			iwpsmethods = strtoul(nvram_value, NULL, 16);
		else {
			/* default configuration methods are same as hostapd's */
			iwpsmethods =
				(WPS_CONFIG_PUSHBUTTON | WPS_CONFIG_VIRT_PUSHBUTTON
				| WPS_CONFIG_PHY_PUSHBUTTON | WPS_CONFIG_DISPLAY
				| WPS_CONFIG_VIRT_DISPLAY);
		}

		remain_size = *plen;
		write_size = 0;
		i = 0;
		while (TRUE) {
			method_str = wldm_wps_supported_config_methods_table[i].str_val;
			if (method_str == NULL)
				break;

			iwpsmethod = wldm_wps_supported_config_methods_table[i].enum_val;
			if ((iwpsmethods & iwpsmethod) == iwpsmethod) {
				ret = snprintf(pvalue + write_size, remain_size,
					(write_size == 0) ? "%s" : ",%s", method_str);
				if (ret < 0) {
					WIFI_ERR("%s: snprintf failed with return value [%d]\n",
						__FUNCTION__, ret);
					return -1;
				} else if (ret >= remain_size) {
					WIFI_ERR("%s: output is truncated\n", __FUNCTION__);
					return -1;
				}
				write_size += ret;
				remain_size -= ret;
			}

			++i;
		}
		WIFI_DBG("%s: get enabled WPS configured method: %s\n", __FUNCTION__, pvalue);
	}

	if (cmd & CMD_SET) {
		AccessPoint_WPS_Object *pObj;

		WIFI_DBG("%s: set enabled WPS configured method: %s\n", __FUNCTION__, pvalue);
		pObj = wldm_get_AccessPointWPSObject(apIndex, AccessPoint_WPS_ConfigMethodsEnabled_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Wps.ConfigMethodsEnabled)
			free(pObj->Wps.ConfigMethodsEnabled);
		pObj->Wps.ConfigMethodsEnabled = strdup(pvalue);
		if (pObj->Wps.ConfigMethodsEnabled) {
			pObj->apply_map |= AccessPoint_WPS_ConfigMethodsEnabled_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvram_name = "wps_config_method", nv_value[BUF_SIZE];
		char *methods, *token;

		methods = strdup(pvalue);
		if (!methods) {
			WIFI_ERR("%s: CMD_SET_NVRAM malloc failed\n", __FUNCTION__);
			return -1;
		}

		iwpsmethods = 0;
		token = strtok(methods, ",");
		while (token != NULL) {
			method_str = wldm_wps_supported_config_methods_table[i].str_val;
			i = 0;
			while (TRUE) {
				method_str = wldm_wps_supported_config_methods_table[i].str_val;
				if (method_str == NULL) {
					WIFI_ERR("%s: unsupported method [%s]\n", __FUNCTION__, token);
					break;
				}

				if (!strcmp(method_str, token)) {
					iwpsmethods |= wldm_wps_supported_config_methods_table[i].enum_val;
					break;
				}

				++i;
			}
			token = strtok(NULL, ",");
		}
		free(methods);

		snprintf(nv_value, sizeof(nv_value), "0x%lx", iwpsmethods);
		WIFI_DBG("%s: nvram set %s=%s\n", __FUNCTION__, nvram_name, nv_value);
		if (wlcsm_nvram_set("wps_config_method", nv_value)) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, nvram_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_WPS_PIN(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "PIN";
	char nvram_name[NVRAM_NAME_SIZE];
	int len;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL );

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid null pvalue\n", __FUNCTION__);
		return -1;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char nvram_name[NVRAM_NAME_SIZE];
		char *nvram_value = NULL;

		if (!plen) {
			WIFI_ERR("%s: invalid null plen\n", __FUNCTION__);
			return -1;
		}

		snprintf(nvram_name, sizeof(nvram_name), "wps_device_pin");
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value == NULL) {
			WIFI_ERR("%s: Null %s \n", __FUNCTION__, nvram_name);
			return -1;
		}

		if (*plen < (strlen(nvram_value) + 1)) {
			WIFI_ERR("%s: Buffer len=%d insufficient for %s\n",
				__FUNCTION__, *plen, nvram_value);
			return -1;
		}
		strncpy(pvalue, nvram_value, strlen(nvram_value));
	}

	if (cmd == CMD_SET) {
		AccessPoint_WPS_Object *pObj;

		pObj = wldm_get_AccessPointWPSObject(apIndex, AccessPoint_WPS_PIN_MASK);
		if (pObj == NULL)
			return -1;

		len = strlen(pvalue);
		if (len != 4 && len != 8) {
			WIFI_ERR("%s: wrong PIN length [%d], expected 4 or 8", __FUNCTION__, len);
			pObj->reject_map |= AccessPoint_WPS_PIN_MASK;
			wldm_rel_Object(pObj, FALSE);
			return -1;
		} else {
			if (pObj->Wps.PIN)
				free(pObj->Wps.PIN);
			pObj->Wps.PIN = strdup(pvalue);
			if (pObj->Wps.PIN) {
				pObj->apply_map |= AccessPoint_WPS_PIN_MASK;
				wldm_rel_Object(pObj, TRUE);
			} else {
				WIFI_ERR("%s malloc failed\n", __FUNCTION__);
				wldm_rel_Object(pObj, FALSE);
				return -1;
			}
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		len = strlen(pvalue);
		if (len != 4 && len != 8) {
			WIFI_ERR("%s: %d: wrong PIN length [%d], expected 4 or 8",
				__FUNCTION__, apIndex, len);
			return -1;
		}
		snprintf(nvram_name, sizeof(nvram_name), "wps_device_pin");
		if (wlcsm_nvram_set(nvram_name, pvalue) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, nvram_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_WPS_Enable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Enable";
	char *nvifname, nvram_name[NVRAM_NAME_SIZE], buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL );

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (!pvalue) {
		WIFI_ERR("%s: %d: invalid null pvalue\n", __FUNCTION__, apIndex);
		return -1;
	}

	nvifname = wldm_get_nvifname(apIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_wps_mode", nvifname);
		*pvalue = nvram_match(nvram_name, "enabled");
	}

	if (cmd == CMD_SET) {
		AccessPoint_WPS_Object *pObj;

		pObj = wldm_get_AccessPointWPSObject(apIndex, AccessPoint_WPS_Enable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Wps.Enable =  *pvalue;
		pObj->apply_map |= AccessPoint_WPS_Enable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(buf, sizeof(buf), "%s", *pvalue ? "enabled" : "disabled");
		snprintf(nvram_name, sizeof(nvram_name), "%s_wps_mode", nvifname);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: %d: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, apIndex, nvram_name, pvalue);
			return -1;
		}
	}

	return 0;
}

/* lan_wps_oob/lan1_wps_oob = disabled => Configured
 * lan_wps_oob/lan1_wps_oob = enabled => Unconfigured
 * default nvram value is enabled
 */
int
wldm_AccessPoint_WPS_ConfigurationState(int cmd, int apIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ConfigurationState";
	char nvram_name[NVRAM_NAME_SIZE];
	char *osifname, *ifnames;
	int i = 0;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {

		osifname = wldm_get_osifname(apIndex);

		/* TODO: only defines lan_wps_oob and lan1_wps_oob so far */
		for (i = 0; i < WLIFU_MAX_NO_BRIDGE; ++i) {
			if (i == 0)
				snprintf(nvram_name, sizeof(nvram_name), "lan_ifnames");
			else
				snprintf(nvram_name, sizeof(nvram_name), "lan%d_ifnames", i);

			ifnames = nvram_safe_get(nvram_name);
			if (find_in_list(ifnames, osifname)) {
				if (i == 0)
					snprintf(nvram_name, sizeof(nvram_name), "lan_wps_oob");
				else
					snprintf(nvram_name, sizeof(nvram_name), "lan%d_wps_oob", i);

				/* Output string is either Not configured or Configured */
				if (nvram_match(nvram_name, "disabled"))
					*plen = snprintf(pvalue, *plen, "Configured") + 1;
				else
					*plen = snprintf(pvalue, *plen, "Not configured") + 1;

				break;
			}
		}
		if (i == WLIFU_MAX_NO_BRIDGE) {
			WIFI_DBG("%s: %s is not in any bridge\n", __FUNCTION__, osifname);
			return -1;
		}
	}

	return 0;
}

/******************************************
*  Device.WiFi.AccessPoint.{i}.Accounting.
******************************************/

int
wldm_AccessPoint_Accounting_Enable(int cmd, int apIndex,
	boolean *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Enable";
	char *nvifname, nvram_name[NVRAM_NAME_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_radius_acct_enabled", nvifname);

		*pvalue = nvram_match(nvram_name, "1") ? TRUE : FALSE;
	}

	if (cmd == CMD_SET) {
		AccessPoint_Accounting_Object *pObj;

		pObj = wldm_get_AccessPointAccountingObject(apIndex,
						AccessPoint_Accounting_Enable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Accounting.Enable =  *pvalue;
		pObj->apply_map |= AccessPoint_Accounting_Enable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_radius_acct_enabled", nvifname);
		if (wlcsm_nvram_set(nvram_name, *pvalue ? "1" : "0") != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n",
				__FUNCTION__, nvram_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Accounting_ServerIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "ServerIPAddr";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *ip_addr = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct_ipaddr", nvifname);

		ip_addr = wlcsm_nvram_get(nv_name);
		if (!ip_addr)
			return -1;

		if (*plen < strlen(ip_addr) + 1) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, ip_addr);
		*plen = strlen(ip_addr) + 1;
	}

	if (cmd & CMD_SET) {
		AccessPoint_Accounting_Object *pObj;

		pObj = wldm_get_AccessPointAccountingObject(apIndex,
			AccessPoint_Accounting_ServerIPAddr_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Accounting.ServerIPAddr) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Accounting.ServerIPAddr);
			free(pObj->Accounting.ServerIPAddr);
		}
		pObj->Accounting.ServerIPAddr = malloc(strlen(pvalue) + 1);
		if (pObj->Accounting.ServerIPAddr) {
			strcpy(pObj->Accounting.ServerIPAddr, pvalue);
			pObj->apply_map |= AccessPoint_Accounting_ServerIPAddr_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct_ipaddr", nvifname);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, pvalue);
		if (wlcsm_nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Accounting_SecondaryServerIPAddr(int cmd, int apIndex,
	IPAddress pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SecondaryServerIPAddr";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *ip_addr = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct2_ipaddr", nvifname);

		ip_addr = wlcsm_nvram_get(nv_name);
		if (!ip_addr)
			return -1;

		if (*plen < strlen(ip_addr) + 1) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, ip_addr);
		*plen = strlen(ip_addr) + 1;
	}

	if (cmd & CMD_SET) {
		AccessPoint_Accounting_Object *pObj;

		pObj = wldm_get_AccessPointAccountingObject(apIndex,
			AccessPoint_Accounting_SecondaryServerIPAddr_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Accounting.SecondaryServerIPAddr) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Accounting.SecondaryServerIPAddr);
			free(pObj->Accounting.SecondaryServerIPAddr);
		}
		pObj->Accounting.SecondaryServerIPAddr = malloc(strlen(pvalue) + 1);
		if (pObj->Accounting.SecondaryServerIPAddr) {
			strcpy(pObj->Accounting.SecondaryServerIPAddr, pvalue);
			pObj->apply_map |= AccessPoint_Accounting_SecondaryServerIPAddr_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct2_ipaddr", nvifname);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, pvalue);
		if (wlcsm_nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Accounting_ServerPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	CHAR *parameter = "ServerPort";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *pPort = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		if (*plen != sizeof(*pvalue)) {
			WIFI_ERR("%s: len is wrong!\n", __FUNCTION__);
			return -1;
		}

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct_port", nvifname);

		pPort = wlcsm_nvram_get(nv_name);
		if (!pPort)
			return -1;

		*pvalue = atoi(pPort);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Accounting_Object *pObj;

		pObj = wldm_get_AccessPointAccountingObject(apIndex,
			AccessPoint_Accounting_ServerPort_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Accounting.ServerPort = *pvalue;
		pObj->apply_map |= AccessPoint_Accounting_ServerPort_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE], nv_value[BUF_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct_port", nvifname);
		snprintf(nv_value, sizeof(nv_value), "%d", *pvalue);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Accounting_SecondaryServerPort(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	CHAR *parameter = "SecondaryServerPort";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *pPort = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		if (*plen != sizeof(*pvalue)) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct2_port", nvifname);

		pPort = wlcsm_nvram_get(nv_name);
		if (!pPort)
			return -1;

		*pvalue = atoi(pPort);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Accounting_Object *pObj;

		pObj = wldm_get_AccessPointAccountingObject(apIndex,
			AccessPoint_Accounting_SecondaryServerPort_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Accounting.SecondaryServerPort = *pvalue;
		pObj->apply_map |= AccessPoint_Accounting_SecondaryServerPort_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE], nv_value[BUF_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct2_port", nvifname);
		snprintf(nv_value, sizeof(nv_value), "%d", *pvalue);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Accounting_Secret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "Secret";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *secret = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct_key", nvifname);

		secret = wlcsm_nvram_get(nv_name);
		if (!secret)
			return -1;

		if (*plen < strlen(secret) + 1) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, secret);
		*plen = strlen(secret) + 1;
	}

	if (cmd & CMD_SET) {
		AccessPoint_Accounting_Object *pObj;

		pObj = wldm_get_AccessPointAccountingObject(apIndex,
			AccessPoint_Accounting_Secret_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Accounting.Secret) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Accounting.Secret);
			free(pObj->Accounting.Secret);
		}
		pObj->Accounting.Secret = malloc(strlen(pvalue) + 1);
		if (pObj->Accounting.Secret) {
			strcpy(pObj->Accounting.Secret, pvalue);
			pObj->apply_map |= AccessPoint_Accounting_Secret_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct_key", nvifname);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, pvalue);
		if (wlcsm_nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Accounting_SecondarySecret(int cmd, int apIndex,
	string pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SecondarySecret";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char* secret = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct2_key", nvifname);

		secret = wlcsm_nvram_get(nv_name);
		if (!secret)
			return -1;

		if (*plen < strlen(secret) + 1) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}
		strcpy(pvalue, secret);
		*plen = strlen(secret) + 1;
	}

	if (cmd & CMD_SET) {
		AccessPoint_Accounting_Object *pObj;

		pObj = wldm_get_AccessPointAccountingObject(apIndex,
			AccessPoint_Accounting_SecondarySecret_MASK);
		if (pObj == NULL)
			return -1;

		if (pObj->Accounting.SecondarySecret) {
			WIFI_DBG("%s: free existing %s %s\n", __FUNCTION__,
				parameter, pObj->Accounting.SecondarySecret);
			free(pObj->Accounting.SecondarySecret);
		}
		pObj->Accounting.SecondarySecret = malloc(strlen(pvalue) + 1);
		if (pObj->Accounting.SecondarySecret) {
			strcpy(pObj->Accounting.SecondarySecret, pvalue);
			pObj->apply_map |= AccessPoint_Accounting_SecondarySecret_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			WIFI_ERR("%s malloc failed\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct2_key", nvifname);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, pvalue);
		if (wlcsm_nvram_set(nv_name, pvalue) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, pvalue);
			return -1;
		}
	}

	return 0;
}

int
wldm_AccessPoint_Accounting_InterimInterval(int cmd, int apIndex,
	unsignedInt *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	CHAR *parameter = "InterimInterval";

	WIFI_DBG("%s cmd=%d apIndex=%d\n", __FUNCTION__, cmd, apIndex);

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char *pInterval = NULL;
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE];

		if (*plen != sizeof(*pvalue)) {
			WIFI_ERR("%s: len too short!\n", __FUNCTION__);
			return -1;
		}

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct_interim_interval", nvifname);

		pInterval = wlcsm_nvram_get(nv_name);
		if (!pInterval)
			return -1;

		*pvalue = atoi(pInterval);
	}

	if (cmd & CMD_SET) {
		AccessPoint_Accounting_Object *pObj;

		pObj = wldm_get_AccessPointAccountingObject(apIndex,
			AccessPoint_Accounting_InterimInterval_MASK);
		if (pObj == NULL)
			return -1;

		pObj->Accounting.InterimInterval = *pvalue;
		pObj->apply_map |= AccessPoint_Accounting_InterimInterval_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char *nvifname = NULL, nv_name[NVRAM_NAME_SIZE], nv_value[BUF_SIZE];

		nvifname = wldm_get_nvifname(apIndex);
		snprintf(nv_name, sizeof(nv_name), "%s_radius_acct_interim_interval", nvifname);
		snprintf(nv_value, sizeof(nv_value), "%d", *pvalue);
		WIFI_DBG("%s wlcsm_nvram_set %s=%s\n", __FUNCTION__, nv_name, nv_value);
		if (wlcsm_nvram_set(nv_name, nv_value) != 0) {
			WIFI_ERR("%s wlcsm_nvram_set %s=%s failed\n", __FUNCTION__, nv_name, nv_value);
			return -1;
		}
	}

	return 0;
}

/*************************************
*  Device.WiFi.AccessPoint.{i}.AC.{j}.
*************************************/

/******** Non TR-181 functions and parameters *************/
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
/* Following two functions are from main/src/wl/exe/wluc_he.c */
static bool
wl_he_get_uint_cb(void *ctx, uint16 *id, uint16 *len)
{
	he_xtlv_v32 *v32 = ctx;

	*id = v32->id;
	*len = v32->len;

	return FALSE;
}

static void
wl_he_pack_uint_cb(void *ctx, uint16 id, uint16 len, uint8 *buf)
{
	he_xtlv_v32 *v32 = ctx;

	BCM_REFERENCE(id);
	BCM_REFERENCE(len);

	v32->val = htod32(v32->val);

	switch (v32->len) {
		case sizeof(uint8):
			*buf = (uint8)v32->val;
			break;
		case sizeof(uint16):
			store16_ua(buf, (uint16)v32->val);
			break;
		case sizeof(uint32):
			store32_ua(buf, v32->val);
			break;
		default:
			/* ASSERT(0); */
			break;
        }
}
#endif /* (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4) */

static int
wl_setHEcmd(unsigned int radioIndex, unsigned short he_id, unsigned short he_len, void *heInfop)
{
	int ret;
	char *osifname;
	char buf[WLC_IOCTL_MEDLEN] = {0};
	char retbuf[WLC_IOCTL_MEDLEN] = {0};
	bcm_xtlv_t *bp;
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
	he_xtlv_v32 v32;
#endif /* (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4) */
	int plen;

	osifname = wldm_get_radio_osifname(radioIndex);

#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
	if (he_id == WL_HE_CMD_COLOR_COLLISION) {
		v32.id = he_id;
		v32.len = he_len;
		v32.val = *(unsigned char *)heInfop;

		ret = bcm_pack_xtlv_buf((void *)&v32, (unsigned char *)buf, sizeof(buf), BCM_XTLV_OPTION_ALIGN32,
			wl_he_get_uint_cb, wl_he_pack_uint_cb, &plen);
		if (ret < 0) {
			WIFI_DBG("%s: bcm_pack_xtlv_buf returned error\n", __FUNCTION__);
		}
	} else
#endif /* (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4) */
	{
		bp = (bcm_xtlv_t *)buf;
		bp->id = he_id;
		bp->len = he_len;

		memcpy((void *)(bp->data), (void *)(heInfop), he_len);

		/* plen = hdr_len (i.e offset to data) + data_len */
		plen = 4 + bp->len;
	}

	/* WIFI_DBG("%s: osifname=%s buf=%p buflen=%d retbuf=%p retbuflen=%d bp->len=%d plen=%d\n",
		__FUNCTION__, osifname, buf, sizeof(buf), retbuf, sizeof(retbuf), bp->len, plen); */

	/* WIFI_ERR("%s: he_id=%d, osifname=%s, data=%d, plen=%d\n", __FUNCTION__, he_id, osifname, bp->data[0], plen); */

	ret = wl_iovar_setbuf(osifname, "he", buf, plen, retbuf, WLC_IOCTL_MEDLEN);
	if (ret < 0) {
		WIFI_DBG("%s: wl_iovar_setbuf failed. osifname=%s, he_id=%d, he_len=%d, ret=%d\n",
			__FUNCTION__, osifname, he_id, he_len, ret);
	}
	return ret;
}

static int
wl_getHEcmd(unsigned int radioIndex, unsigned short he_id, unsigned short he_len, void *heInfop)
{
	int ret = -1;
	char *osifname;
	char buf[WLC_IOCTL_MEDLEN] = {0};
	char retbuf[WLC_IOCTL_MEDLEN] = {0};
	bcm_xtlv_t *bp;
	int plen;

	if (heInfop == NULL) {
		WIFI_DBG("%s: fail NULL heInfop ret=%d \r\n", __FUNCTION__, ret);
		return(ret);
	}

	osifname = wldm_get_radio_osifname(radioIndex);
	bp = (bcm_xtlv_t *)buf;
	bp->id = he_id;
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
	if (he_id == WL_HE_CMD_TRIGGER_COLOR_EVENT) {
		bp->len = he_len;
	} else
#endif /* End of (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4) */
	{
		bp->len = 0;
	}
	/* plen = hdr_len (i.e offset to data) + data_len */
	plen = 4 + bp->len;

	/* WIFI_DBG("%s: osifname=%s buf=%p buflen=%d retbuf=%p retbuflen=%d plen=%d\n",
		__FUNCTION__, osifname, buf, sizeof(buf), retbuf, sizeof(retbuf), plen); */

	ret = wl_iovar_getbuf(osifname, "he", buf, plen, retbuf, sizeof(retbuf));
	if (ret < 0) {
		WIFI_DBG("%s: wl_iovar_getbuf failed. osifname=%s, he_id=%d, he_len=%d, ret=%d\n",
			__FUNCTION__, osifname, he_id, he_len, ret);
	} else {
		memcpy((void *)heInfop, (void *)(&retbuf[0]), he_len);
	}
	return ret;
}

#ifdef WL_TWT_LIST_VER
static int
wl_HE_TWTcmd(unsigned int apIndex, unsigned short twt_id, void *tinp, int len,
		void *tout, int *outlen)
{
	char *osifname;
	unsigned char buf[WLC_IOCTL_SMLEN] = {0};
	char retbuf[WLC_IOCTL_MEDLEN] = {0};
	wldm_twt_list_t *list;
	wldm_twt_sdesc_t *dp;
	bcm_xtlv_t *bp;
	int ret = -1, plen, totlen, num, i, *twt_en;

	if (((twt_id == WL_TWT_CMD_LIST) || (twt_id == WL_TWT_CMD_TEARDOWN) ||
		(twt_id == WL_TWT_CMD_SETUP)) &&
		((tinp == NULL) || (len > (sizeof(buf) - 4)))) {
		WIFI_ERR("%s: Error in input param apIndex=%d twt_id=0x%x\n",
			__FUNCTION__, apIndex, twt_id);
		return -1;
	}
	if (((twt_id == WL_TWT_CMD_ENAB) || (twt_id == WL_TWT_CMD_LIST)) && (tout == NULL)) {
		WIFI_ERR("%s: Error in output param apIndex=%d twt_id=0x%x\n",
			__FUNCTION__, apIndex, twt_id);
		return -1;
	}
	osifname = wldm_get_osifname(apIndex);
	memset(buf, 0, sizeof(buf));
	memset(retbuf, 0, sizeof(retbuf));
	bp = (bcm_xtlv_t *)buf;
	bp->id = twt_id;
	bp->len = len;
	memcpy(((void *)(bp->data)), (void *)(tinp), len);

	/* plen = hdr_len (i.e offset to data) + data_len */
	plen = 4 + bp->len;

	if (twt_id == WL_TWT_CMD_ENAB) {
		ret = wl_iovar_getbuf(osifname, "twt", buf, plen, retbuf, sizeof(retbuf));
		if (ret < 0) {
			WIFI_ERR("%s: %s wl_iovar_getbuf failed. twt_id=%d, plen=%d, ret=%d\n",
			__FUNCTION__, osifname, twt_id, plen, ret);
			return ret;
		}
		twt_en = (int *)tout;
		*twt_en = (int)(retbuf[0]);
		*outlen = sizeof(*twt_en);
	}
	else if (twt_id == WL_TWT_CMD_LIST) {
		unsigned int wake_dur_unit = 256;

		ret = wl_iovar_getbuf(osifname, "twt", buf, plen, retbuf, sizeof(retbuf));
		if (ret < 0) {
			WIFI_ERR("%s: %s wl_iovar_getbuf failed. twt_id=%d, plen=%d, ret=%d\n",
				__FUNCTION__, osifname, twt_id, plen, ret);
			return ret;
		}
		list = (wldm_twt_list_t *)(&retbuf[0]);
		num = list->bcast_count + list->indv_count;
		totlen = sizeof(wldm_twt_list_t) + (num * sizeof(wldm_twt_sdesc_t));
		WIFI_DBG("%s %s twt list ver=%d bcast_count=%d indv_count=%d num=%d "
			"totlen=%d retbufsize=%d outlen=%d\n",
			__FUNCTION__, osifname, list->version, list->bcast_count,
			list->indv_count, num, totlen, sizeof(retbuf), *outlen);
		if (totlen > (*outlen)) {
			WIFI_ERR("%s: %s retbuf size=%d too small, need totlen=%d for %d clients\n",
				__FUNCTION__, osifname, sizeof(retbuf), totlen, num);
			return -1;
		}
		dp = list->desc;
		i = 0;
		while (num > 0) {
#if (WL_TWT_LIST_VER >= 2)
			wake_dur_unit = dp->wake_duration_unit ?  1024 : 256;
#endif /* WL_TWT_LIST_VER */
			WIFI_DBG("%d dp=%p TWT: id=%2d wake_int=%6d wake_dur=%5d "
				"chan=0x%02x u=%s t=%s p=%s type=%s\n",	num, dp, dp->id,
				dp->wake_interval_mantissa * (1 << dp->wake_interval_exponent),
				wake_dur_unit * dp->wake_duration, dp->channel,
				(dp->flow_flags & WL_TWT_FLOW_FLAG_UNANNOUNCED) ? "YES" : "NO ",
				(dp->flow_flags & WL_TWT_FLOW_FLAG_TRIGGER) ? "YES" : "NO ",
				(dp->flow_flags & WL_TWT_FLOW_FLAG_PROTECTION) ? "YES" : "NO ",
				(i++ < list->bcast_count) ? "bcast" : "indiv");
			dp++; num--;
		}
		memcpy((void *)tout, (void *)(&retbuf[0]), totlen);
		*outlen = totlen;
	}
	else if (twt_id == WL_TWT_CMD_TEARDOWN) {
		ret = wl_iovar_setbuf(osifname, "twt", buf, plen, retbuf, sizeof(retbuf));
		if (ret < 0) {
			WIFI_ERR("%s: %s wl_iovar_setbuf failed. twt_id=%d, plen=%d, ret=%d\n",
				__FUNCTION__, osifname, twt_id, plen, ret);
			return ret;
		}
	}
	else if (twt_id == WL_TWT_CMD_SETUP) {
		ret = wl_iovar_setbuf(osifname, "twt", buf, plen, retbuf, sizeof(retbuf));
		if (ret < 0) {
			WIFI_ERR("%s: %s wl_iovar_setbuf failed. twt_id=%d, plen=%d, ret=%d\n",
				__FUNCTION__, osifname, twt_id, plen, ret);
			return ret;
		}
	}

	return ret;
}

static int
wl_getHE_TWTEnable(unsigned int apIndex, int *enable)
{
	int ret, len;

	len = sizeof(*enable);
	ret = wl_HE_TWTcmd(apIndex, WL_TWT_CMD_ENAB, NULL, 0, (void *)(enable), &len);
	return ret;
}

static void
wl_twt_sdesc_to_wldm_twt_params(wldm_twt_sdesc_t *dp, wldm_twt_params_t *tparamp)
{
	wldm_twt_individual_params_t *tpi;
	wldm_twt_broadcast_params_t *tpb;
	unsigned int wake_dur_unit = 256;
#if (WL_TWT_LIST_VER >= 2)
	wake_dur_unit = dp->wake_duration_unit ?  1024 : 256;
#endif /* WL_TWT_LIST_VER */

	tparamp->agreement = (dp->flow_flags & WL_TWT_FLOW_FLAG_BROADCAST) ?
		wldm_twt_agreement_type_broadcast : wldm_twt_agreement_type_individual;
	tparamp->operation.implicit = (dp->flow_flags & WL_TWT_FLOW_FLAG_IMPLICIT) ? 1 : 0;
	tparamp->operation.announced = (dp->flow_flags & WL_TWT_FLOW_FLAG_UNANNOUNCED) ? 0 : 1;
	tparamp->operation.trigger_enabled = (dp->flow_flags & WL_TWT_FLOW_FLAG_TRIGGER) ? 1 : 0;
	if (tparamp->agreement == wldm_twt_agreement_type_individual) {
		tpi = &(tparamp->params.individual);
		tpi->wakeTime_uSec = wake_dur_unit * dp->wake_duration;
		tpi->wakeInterval_uSec = dp->wake_interval_mantissa * (1 << dp->wake_interval_exponent);
		tpi->minWakeDuration_uSec = 0;
		tpi->channel = dp->channel;
	}
	else if (tparamp->agreement == wldm_twt_agreement_type_broadcast) {
		/* TBD */
		tpb = &(tparamp->params.broadcast);
		tpb->target_beacon_uSec = 0;
		tpb->listen_interval_uSec = 0;
	}
}

static int
wl_getHE_TWTParams(unsigned int apIndex, struct ether_addr *peer_mac, void *tout, int *outlen)
{
	int ret, enable;
	wldm_twt_list_t tinp;
	wldm_twt_list_t *toutp = (wldm_twt_list_t *)(tout);

	ret = wl_getHE_TWTEnable(apIndex, &enable);
	if ((ret < 0) || !(enable)) {
		WIFI_DBG("%s: TWT not enabled apIndex=%d\n", __FUNCTION__, apIndex);
		*outlen = 0;
		return 0;
	}
	bzero(&tinp, sizeof(tinp));
	tinp.version = WL_TWT_LIST_VER;
	tinp.length = sizeof(tinp) - (sizeof(tinp.version) + sizeof(tinp.length));
	if (peer_mac) {
		tinp.peer = *peer_mac;
	}

	ret = wl_HE_TWTcmd(apIndex, WL_TWT_CMD_LIST, (void *)(&tinp), sizeof(tinp),
		(void *)(tout), outlen);
	if (ret < 0) {
		WIFI_ERR("%s: failed. apIndex=%d ret=%d *outlen=%d\n",
			__FUNCTION__, apIndex, ret, *outlen);
		return -1;
	}
	toutp->peer = *peer_mac;
	return 0;
}

static int
wl_getHE_TWTSessions(unsigned int apIndex, void *tout, int *outlen)
{
	int ret, i, alen = sizeof(wldm_twt_list_t), tcnt = 0, totlen = *outlen, retlen = 0;
	char assoclist_buf[WLC_IOCTL_MEDLEN];
	struct maclist *assoclist = (struct maclist *)assoclist_buf;
	char retbuf[WLC_IOCTL_MEDLEN] = {0};
	struct ether_addr *ea;
	char *osifname = wldm_get_osifname(apIndex);
	wldm_twt_list_all_t *tallp = (wldm_twt_list_all_t *)tout;
	wldm_twt_list_t *tlist = (wldm_twt_list_t *)(tallp->tlist);

	ret = wl_get_assoclist(osifname, assoclist_buf, sizeof(assoclist_buf));
	if (ret < 0) {
		WIFI_ERR("%s: wl_get_assoclist failed apIndex=[%d], ret=[%d]\n",
			__FUNCTION__, apIndex, ret);
		return ret;
	}
	if (assoclist->count == 0) {
		WIFI_DBG("%s: No associated devices apIndex=[%d], ret=[%d]\n",
			__FUNCTION__, apIndex, ret);
		tallp->devcnt = 0;
		return 0;
	}

	for (i = 0, ea = assoclist->ea; i < assoclist->count; i++, ea++) {
		alen = sizeof(retbuf);
		if (wl_getHE_TWTParams(apIndex, ea, (void *)(retbuf), &alen) == 0) {
			if (alen == 0) {
				WIFI_DBG("%s: apIndex[%d] alen=0\n", __FUNCTION__, apIndex);
				continue;
			}
			/* copy to out buffer */
			if (totlen < alen) {
				WIFI_ERR("%s: apIndex[%d] Buffer short got=%d need=%d outlen=%d\n",
					__FUNCTION__, apIndex, totlen, alen, *outlen);
				return -1;
			}
			memcpy((char *)tlist, (char *)(retbuf), alen);
			tlist = (wldm_twt_list_t *)((char *)(tlist) + alen);
			tcnt++;
			totlen -= alen;
			retlen += alen;
		}
	}
	tallp->devcnt = tcnt;
	*outlen = retlen + sizeof(wldm_twt_list_all_t);
	WIFI_DBG("%s: apIndex[%d] tcnt=%d retlen=%d twt sessions\n",  __FUNCTION__, apIndex, tcnt, *outlen);

	return 0;
}

static int
wl_teardownHE_TWT(unsigned int apIndex, wldm_twt_sess_info_t *tsp)
{
	wl_twt_teardown_t ttear = {0};
	int ret;

	ttear.peer = tsp->peer;
	ttear.id = tsp->twtId;
	ttear.flow_flags = (tsp->agtype == wldm_twt_agreement_type_broadcast) ?
		WL_TWT_FLOW_FLAG_BROADCAST : 0;
	ttear.version = WL_TWT_TEARDOWN_VER;
	ttear.length = sizeof(ttear) - (sizeof(ttear.version) + sizeof(ttear.length));
	ret = wl_HE_TWTcmd(apIndex, WL_TWT_CMD_TEARDOWN, (void *)(&ttear),
		sizeof(ttear), NULL, 0);
	if (ret < 0) {
		WIFI_ERR("%s: WL_TWT_CMD_TEARDOWN idx=%d, ret=%d\n",
			__FUNCTION__, apIndex, ret);
		return -1;
	}
	return 0;
}

static void
wl_twt_bcast_uSec_to_mantissa_exponent(unsigned int wake_interval,
	unsigned short *wake_interval_mantissa,
	unsigned char *wake_interval_exponent)
{
	if (wake_interval & WLDM_TSFL_SCHEDID_MASK_INV) {
		wake_interval &= WLDM_TSFL_SCHEDID_MASK;
		if (wake_interval == 0) {
			wake_interval = (1 << WLDM_SCHEDID_TSF_SHIFT);
		}
		wake_interval += (1 << WLDM_SCHEDID_TSF_SHIFT);
	}
	*wake_interval_exponent = WLDM_SCHEDID_TSF_SHIFT;
	*wake_interval_mantissa = wake_interval >> WLDM_SCHEDID_TSF_SHIFT;
	return;
}

int
wl_setupHE_TWT(unsigned int apIndex, wldm_twt_setup_info_t *tsip)
{
#if (WL_TWT_SETUP_VER == 2)
	wl_twt_setup_v2_t tsetup = {0};
	unsigned int max_wake_dur_tuints = 1023;
#else
	wl_twt_setup_t tsetup = {0};
	unsigned int max_wake_dur_tuints = 255;
#endif /* WL_TWT_SETUP_VER == 2 */

	wldm_twt_params_t *tpar = &(tsip->tparams);
	int ret;
	unsigned int wake_duration;

	tsetup.version = WL_TWT_SETUP_VER;
	tsetup.length = sizeof(tsetup) - (sizeof(tsetup.version) + sizeof(tsetup.length));
	tsetup.desc.id = (tsip->sessId > 0) ? tsip->sessId : WL_TWT_ID_BCAST_AUTO;
	tsetup.desc.flow_flags = WL_TWT_FLOW_FLAG_BROADCAST;
	/* TBD - chk implicit vs protection in driver (tpar>operation.implicit) */
	tsetup.desc.flow_flags |= (tpar->operation.announced) ? 0 : WL_TWT_FLOW_FLAG_UNANNOUNCED;
	tsetup.desc.flow_flags |= (tpar->operation.trigger_enabled) ? WL_TWT_FLOW_FLAG_TRIGGER : 0;

	wake_duration = tpar->params.broadcast.target_beacon_uSec;
	if (wake_duration & WLDM_TSFL_SCHEDID_MASK_INV) {
		wake_duration &= WLDM_TSFL_SCHEDID_MASK;
		wake_duration += WLDM_SCHEDID_TO_TSFL(1);
		/* Check for exceeding max wake_duration of 1023 (*256usec) */
		if (wake_duration > (max_wake_dur_tuints << 8)) {
			wake_duration -= WLDM_SCHEDID_TO_TSFL(1);
		}
	}

#if (WL_TWT_SETUP_VER == 2)
	/* Is duration > 64 msec? */
	if (wake_duration >= WLDM_TWT_WAKE_64k_USEC) {
		/* Make it multiple of 16384 usec */
		wake_duration &= ~(WLDM_SCHEDID_TO_TSFL(4) - 1);
		tsetup.desc.wake_duration = wake_duration >> 10;
		tsetup.desc.wake_duration_unit = 1;
	} else {
		tsetup.desc.wake_duration = wake_duration >> 8;
		tsetup.desc.wake_duration_unit = 0;
	}
#else
	tsetup.desc.wake_duration = wake_duration >> 8;
#endif /* WL_TWT_SETUP_VER == 2 */

	WIFI_DBG("%s: ap%d target_beacon_uSec=%d div256=%d wake_duration=%d tsetup_dur=%d MIN=%d\n",
		__FUNCTION__, apIndex, tpar->params.broadcast.target_beacon_uSec,
		tpar->params.broadcast.target_beacon_uSec / 256, wake_duration,
		tsetup.desc.wake_duration, WLDM_TWT_MIN_WAKE_DURATION);

	wl_twt_bcast_uSec_to_mantissa_exponent(tpar->params.broadcast.listen_interval_uSec,
		&(tsetup.desc.wake_interval_mantissa), &(tsetup.desc.wake_interval_exponent));

	WIFI_DBG("%s: ap%d ver=%d len=%d desc cmd=%d id=%d fflag=0x%x dur=%d intvl m=%d exp=%d\n",
		__FUNCTION__, apIndex, tsetup.version, tsetup.length, tsetup.desc.setup_command,
		tsetup.desc.id, tsetup.desc.flow_flags, tsetup.desc.wake_duration,
		tsetup.desc.wake_interval_mantissa, tsetup.desc.wake_interval_exponent);

	ret = wl_HE_TWTcmd(apIndex, WL_TWT_CMD_SETUP, (void *)(&tsetup),
		sizeof(tsetup), NULL, 0);
	if (ret < 0) {
		WIFI_ERR("%s: WL_TWT_SETUP_CMD_REQUEST idx=%d, ret=%d\n",
			__FUNCTION__, apIndex, ret);
		return -1;
	}
	return 0;
}
#endif /* WL_TWT_LIST_VER */

int
wldm_AXenable(int cmd, unsigned int radioIndex,
	boolean *result, int *plen, char *pbuf, int *pbufsz)
{
	int ret = 0;

	char *parameter = "HEenabled";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_NVRAM | CMD_GET_NVRAM);
	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		ret = wl_getHEcmd(radioIndex, WL_HE_CMD_ENAB, sizeof(result), (void *) result);
		if (ret < 0) {
			WIFI_DBG("%s: wl_getHEcmd Failed. radioIndex=%d, HEenable=%d, ret=%d\n",
				__FUNCTION__, radioIndex, *result, ret);
		}
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
							X_BROADCOM_COM_Radio_AxEnable_MASK);
		if (pObj == NULL)
			return -1;

		pObj->X_BROADCOM_COM_Radio.AxEnable = *result ? 1 : 0;
		pObj->apply_map |= X_BROADCOM_COM_Radio_AxEnable_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_IOCTL) {
		ret = wl_setHEcmd(radioIndex, WL_HE_CMD_ENAB, sizeof(result), result);
		if (ret < 0) {
			WIFI_DBG("%s: wl_setHEcmd Failed. radioIndex=%d, HEenable=%d, ret=%d\n",
				__FUNCTION__, radioIndex, *result, ret);
		}
	}
	return ret;
}

int
wldm_AXfeatures(int cmd, unsigned int radioIndex,
	unsigned int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	int ret=0;
	char nvrVarName[STRING_LENGTH_32] = {0};
	char nvrValStr[STRING_LENGTH_32] = {0};
	char *parameter = "HEfeatures";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);
	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		ret = wl_getHEcmd(radioIndex, WL_HE_CMD_FEATURES, sizeof(uint32), (void *)(pvalue));
		if (ret < 0) {
			WIFI_ERR("%s: wl_getHEcmd Failed. radioIndex=%d, HEfeatures=0x%x, ret=%d\n",
				__FUNCTION__, radioIndex, (*pvalue), ret);
			return -1;
		}
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
			X_BROADCOM_COM_Radio_AxFeatures_MASK);
		if (pObj == NULL)
			return -1;

		pObj->X_BROADCOM_COM_Radio.AxFeatures = *pvalue;   /* Need to validate input later */
		pObj->apply_map |= X_BROADCOM_COM_Radio_AxFeatures_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_IOCTL) {
		uint32 enable = 0;
		ret = wl_setHEcmd(radioIndex, WL_HE_CMD_FEATURES, sizeof(uint32), pvalue);
		if (ret < 0) {
			WIFI_ERR("%s: wl_setHEcmd Failed. radioIndex=%d, HEfeatures=%d, ret=%d\n",
				__FUNCTION__, radioIndex, (*pvalue), ret);
			return -1;
		}
		enable = (*pvalue != 0) ? 1 : 0;
		if (wl_setHEcmd(radioIndex, WL_HE_CMD_ENAB, sizeof(enable), &enable) < 0) {
			WIFI_ERR("%s: wl_setHEcmd he %d Failed\n", __FUNCTION__, enable);
			return -1;
		}
		WIFI_DBG("%s: Done, send he feature 0x%x to driver\n", __FUNCTION__, *pvalue);
	}

	if (cmd & CMD_SET_NVRAM) {
		/* save config in nvram */
		snprintf(nvrVarName, sizeof(nvrVarName), "wl%d_he_features", radioIndex);
		snprintf(nvrValStr, sizeof(nvrValStr), "%d", *pvalue);
		ret = wlcsm_nvram_set(nvrVarName, (char *)nvrValStr);
		if (!ret) {
			WIFI_DBG("%s: Done, radioIndex=%d pvalue=0x%x %s=%s\n",
				__FUNCTION__, radioIndex, *pvalue, nvrVarName, nvrValStr);
		} else {
			WIFI_ERR("%s: setting %s=%s ret=%d failed\n",
				__FUNCTION__, nvrVarName, nvrValStr, ret);
		}
	}
	return ret;
}

static int
wl_setBSSColorInfo(unsigned int radioIndex, wl_he_bsscolor_t *bcp)
{
	int ret;

	ret = wl_setHEcmd(radioIndex, WL_HE_CMD_BSSCOLOR, sizeof(wl_he_bsscolor_t), (void *)(bcp));
	if (ret < 0) {
		WIFI_ERR("%s: wl_setHEcmd failed. radioIndex=%d, bsscolor=%d, dis=%d, sc=%d, ret=%d\n",
			__FUNCTION__, radioIndex, bcp->color, bcp->disabled, bcp->switch_count, ret);
	}
	return ret;
}

static int
wl_getBSSColorInfo(unsigned int radioIndex, wl_he_bsscolor_t *bcp)
{
	int ret;

	ret = wl_getHEcmd(radioIndex, WL_HE_CMD_BSSCOLOR, sizeof(wl_he_bsscolor_t), (void *)(bcp));
	if (ret < 0) {
		WIFI_ERR("%s: wl_getHEcmd failed. radioIndex=%d, bsscolor=%d, dis=%d, sc=%d, ret=%d\n",
			__FUNCTION__, radioIndex, bcp->color, bcp->disabled, bcp->switch_count, ret);
	}
	return ret;
}

int
wldm_AXbssColor(int cmd, unsigned int radioIndex,
	unsigned int *color, int *plen, char *pbuf, int *pbufsz)
{
        int ret = 0, ret1 = 0;
        wl_he_bsscolor_t curr_bc;
        wl_he_bsscolor_t *bcp;
	char *parameter = "BSSColor";
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
	unsigned char color_collision = 0;
#endif

        /* WIFI_DBG("%s: radioIndex=%d color=%d\n", __FUNCTION__, radioIndex, *color); */

        bcp = &curr_bc;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_GET_NVRAM | CMD_SET_NVRAM);
	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		ret = wl_getBSSColorInfo(radioIndex, &curr_bc);
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
		ret1 = wl_getHEcmd(radioIndex, WL_HE_CMD_COLOR_COLLISION, sizeof(color_collision),
				(void *)&color_collision);
#endif
		if ((ret1 == 0) && (ret == 0)) {
			/* return 0xff:0xff Color Collision enabled:color to hal */
			*color = 0;
			*color |= curr_bc.color;
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
			if (color_collision != 0) {
#else
			if (!curr_bc.disabled) {
#endif
				*color |= 1 << 8;
			}
		} else {
			if (ret < 0) {
				WIFI_ERR("%s: wl_getBSSColorInfo Failed. radioIndex=%d,"
					"*color=%d ret=%d\n", __FUNCTION__, radioIndex, *color, ret);
			}
			if (ret1 < 0) {
				WIFI_ERR("%s: wl_getHEcmd Failed. radioIndex=%d,"
					"*color=%d ret=%d\n", __FUNCTION__, radioIndex, *color, ret1);
			}
		}
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
			X_BROADCOM_COM_Radio_AxBsscolor_MASK);
		if (pObj == NULL)
			return -1;

		if ((*color >= 0) && (*color < 64)) {
			pObj->X_BROADCOM_COM_Radio.AxBsscolor = *color;
			pObj->apply_map |= X_BROADCOM_COM_Radio_AxBsscolor_MASK;
			wldm_rel_Object(pObj, TRUE);
		} else {
			pObj->reject_map |= X_BROADCOM_COM_Radio_AxBsscolor_MASK;
			WIFI_ERR("%s: wrong value for color=%d\n", __FUNCTION__, *color);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		/* Get current bsscolor */
		ret = wl_getBSSColorInfo(radioIndex, bcp);
		if (ret < 0) {
			WIFI_ERR("%s: wl_getBSSColorInfo Failed. radioIndex=%d, ret=%d\n",
				__FUNCTION__, radioIndex, ret);
			return ret;
		}

		/* if set color is 0 - implies disabled */
		if (*color == 0) {
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
			color_collision = 0;      /* set disabled to TRUE - donot change color */
#else
			bcp->disabled = 1;	 /* set disabled to TRUE - donot change color */
#endif
		} else {
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
			color_collision = HE_CC_AP_DETECT_ENAB | HE_CC_AP_REPORT_HANDLER_ENAB |
				HE_CC_AUTO_ENAB | HE_CC_STA_DETECT_ENAB;
			bcp->color = *color;
#else
			bcp->disabled = 0;
#endif
		}
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
		WIFI_DBG("%s: radioIndex=%d call wl_setBSSColorInfo & wl_setHEcmd"
			" bcp->color=%d Color_Collision=%d sc=%d\n", __FUNCTION__, radioIndex,
			bcp->color, color_collision, bcp->switch_count);
#else
		WIFI_DBG("%s: radioIndex=%d call wl_setBSSColorInfo & wl_setHEcmd"
			" bcp->color=%d sc=%d\n", __FUNCTION__, radioIndex,
			bcp->color, bcp->switch_count);
#endif
		ret = wl_setBSSColorInfo(radioIndex, bcp);
		if (ret < 0) {
			WIFI_ERR("%s: wl_setBSSColorInfo Failed. radioIndex=%d, ret=%d\n",
				__FUNCTION__, radioIndex, ret);
		}
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
		ret1 = wl_setHEcmd(radioIndex, WL_HE_CMD_COLOR_COLLISION, sizeof(color_collision),
			(void *) &color_collision);
		if (ret1 < 0) {
			WIFI_ERR("%s: wl_setHEcmd Failed. radioIndex=%d, ret=%d\n",
				__FUNCTION__, radioIndex, ret1);
		}
#endif
	}
	return (ret | ret1);
}

int
wldm_AXavailableBssColors(int cmd, unsigned int radioIndex,
	unsigned char *color_list, int *plen, char *pbuf, int *pbufsz)
{
#if (SCB_RX_REPORT_DATA_STRUCT_VERSION >= 4)
        int ret = 0, i, count = 0, k, found;
	wl_color_event_t color;
	char *parameter = "AvailableBSSColors";

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_GET_NVRAM | CMD_SET_NVRAM |
		 CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		if (*plen < WL_COLOR_MAX_VALUE) {
			WIFI_ERR("%s: list length is < %d\n",
				__FUNCTION__, WL_COLOR_MAX_VALUE);
			return -1;
		}
		ret = wl_getHEcmd(radioIndex, WL_HE_CMD_TRIGGER_COLOR_EVENT, sizeof(color),
			(void *) &color);
		if (ret < 0) {
			WIFI_ERR("%s: wl_getHEcmd Failed, radioIndex=%d, ret=%d\n",
				__FUNCTION__, radioIndex, ret);
			return -1;
		} else {
			for (i = 0; i < WL_COLOR_MAX_VALUE; i++) {
				found = 0;
				for (k = 0; k < ARRAY_SIZE(color.colors); k++) {
					if (color.colors[k] == (i + 1)) {
						found = 1;
						break;
					}
				}
				if (!found) {
					*(color_list + count++) = i + 1;
				}
			}
		}
	}
	return count;
#else
	WIFI_ERR("%s: Not supported, WiFi must be atleast 21.1 or above\n", __FUNCTION__);
	return -1;
#endif
}

/* Use to support wl mode_reqd [-C bss_idx ] [value] - Current use only for setRadioMode */
/* TBD - update as needed and add to X_BROADCOM_AccesPoint */
static int
dm_mode_reqd(int cmd, int apIndex, int *pvalue, int *plen, char *pvar)
{
	char *nvifname, *osifname, buf[BUF_SIZE];

        if (strcmp(pvar, "mode_reqd") != 0) {
                WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
                return -1;;
        }

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);
	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char nvram_name[NVRAM_NAME_SIZE];
		char *nvram_value = NULL;

		snprintf(nvram_name, sizeof(nvram_name), "%s_bss_opmode_cap_reqd", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value == NULL) {
			WIFI_ERR("%s: Null %s \n", __FUNCTION__, nvram_name);
			return -1;
		}
		*pvalue = atoi(nvram_value);
		*plen = sizeof(*pvalue);
	}

	if (cmd & (CMD_SET_NVRAM | CMD_SET_IOCTL)) {
		if ((pvalue == NULL) || (*pvalue < 0) || (*pvalue >= OMC_MAX)) {
			WIFI_ERR("%s: Error pvalue\n", __FUNCTION__);
			return -1;
		}
	}

	if (cmd & (CMD_SET_NVRAM)) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_bss_opmode_cap_reqd", nvifname);
		snprintf(buf, sizeof(buf), "%d", *pvalue);
		NVRAM_SET(nvram_name, buf);
	}

	if (cmd & CMD_SET_IOCTL) {
		/* wl mode_reqd [-C bss_idx ] [value] - TBD use ioctl */
		/* int bssidx = wldm_get_bssidx(apIndex);
		 * wl_bssiovar_setint(osifname, "mode_reqd", bssidx, *pvalue); */
		snprintf(buf, sizeof(buf), "wl -i %s mode_reqd %d", osifname, *pvalue);
		WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
		if (system(buf) != 0) {
			WIFI_ERR("%s: Error setting %s mode_reqd\n", __FUNCTION__, osifname);
			return -1;
		}
	}
	return 0;
}

static xbrcm_t xbrcm_11ac_tbl[] = {
	{  "vhtmode",	{dm_vhtmode},	CMD_GET | CMD_SET_IOCTL | CMD_GET_NVRAM | CMD_SET_NVRAM,	},
	{  NULL,	{NULL},		0,								},
};

int
wldm_xbrcm_11ac(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);

	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_11ac_tbl);
}

/* Use to support wl vhtmode [value] - Current use only for setRadioMode */
/* TBD - update as needed and add to X_BROADCOM_Radio */
static int
dm_vhtmode(int cmd, int radioIndex, int *pvalue, int *plen, char *pvar)
{
	char *nvifname, *osifname, buf[BUF_SIZE];

	if (strcmp(pvar, "vhtmode") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);
	osifname = wldm_get_radio_osifname(radioIndex);
	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char nvram_name[NVRAM_NAME_SIZE];
		char *nvram_value = NULL;

		snprintf(nvram_name, sizeof(nvram_name), "%s_vhtmode", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value == NULL) {
			WIFI_ERR("%s: Null %s \n", __FUNCTION__, nvram_name);
			return -1;
		}
		*pvalue = atoi(nvram_value);
		*plen = sizeof(*pvalue);
	}

	if (cmd & (CMD_SET_NVRAM | CMD_SET_IOCTL)) {
		if ((pvalue == NULL) || ((*pvalue != AUTO_MODE) &&
					 (*pvalue != ON) && (*pvalue != OFF))) {
			WIFI_ERR("%s: Error pvalue\n", __FUNCTION__);
			return -1;
		}
	}
	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		snprintf(nvram_name, sizeof(nvram_name), "%s_vhtmode", nvifname);
		snprintf(buf, sizeof(buf), "%d", *pvalue);
		NVRAM_SET(nvram_name, buf);
	}

	if (cmd & CMD_SET_IOCTL) {
		/* wl vhtmode [value] - TBD try ioctl */
		/* wl_iovar_setint(osifname, "vhtmode", pvalue); */
		snprintf(buf, sizeof(buf), "wl -i %s vhtmode %d", osifname, *pvalue);
		WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
		if (system(buf) != 0) {
			WIFI_ERR("%s: Error setting %s vhtmode \n", __FUNCTION__, osifname);
			return -1;
		}
	}
	return 0;
}

/* Remove spaces in string */
static boolean
remSpaceInStr(char *out_str, int out_len, const char *in_str)
{
	int i, j;
	int len = strlen(in_str);

	if (out_len < (len + 1))
		return FALSE;
	for (i = 0, j = 0; i < len; i++) {
		if (in_str[i] != ' ')
			out_str[j++] = in_str[i];
	}
	out_str[j] = '\0';
	WIFI_DBG("In %s out_str is %s\n", __FUNCTION__, out_str);
	return TRUE;
}

/* Check if stds string is valid - these are usually from hal api to get or setRadioMode */
static boolean
validStdsStr(char *aStr)
{
	char *stdsStrs[] = {"a", "b", "g", "n", "g,n", "b,g,n", "ac", "n,ac", "a,n,ac", "ax", "g,n,ax", "a,n,ac,ax",
			"n,ax", "b,g,n,ax", "ac,ax", "n,ac,ax", NULL};
	int i = 0;

	while (stdsStrs[i] != NULL) {
		if (strcmp(stdsStrs[i], aStr) == 0) {
			WIFI_DBG("In %s Found %s\n", __FUNCTION__, aStr);
			return TRUE;
		}
		else {
			i++;
		}
	}
	WIFI_DBG("In %s Not Found %s\n", __FUNCTION__, aStr);
	return FALSE;
}

/* Example usage: wldm_Radio_OperatingStandards(CMD_SET, radioIndex, operatingStandards, &len, NULL, NULL) */
/* TBD - follow TR181 path name: wlX_Radio_OperatingStandards */
int
wldm_Radio_OperatingStandards(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "OperatingStandards";
	char *nvifname;
	char out_str[MAX_STDSTR_LEN];
	int slen;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_radio_nvifname(radioIndex);

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		char nvram_name[NVRAM_NAME_SIZE];
		char *nvram_value = NULL;

		snprintf(nvram_name, sizeof(nvram_name), "%s_oper_stands", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value == NULL) {
			WIFI_ERR("%s: Null %s \n", __FUNCTION__, nvram_name);
			return -1;
		}
		if (*plen < (strlen(nvram_value) + 1)) {
			WIFI_ERR("%s: Buffer len=%d insufficient for %s\n",
				__FUNCTION__, *plen, nvram_value);
			return -1;
		}
		*plen = strlen(nvram_value) + 1;
		memcpy(pvalue, nvram_value, *plen);
	}

	if (cmd & CMD_SET) {
		Radio_Object *pObj = wldm_get_RadioObject(radioIndex,
			Radio_OperatingStandards_MASK);
		if (pObj == NULL) {
			WIFI_ERR("%s: Invalid Radio_Object\n", __FUNCTION__);
			return -1;
		}

		if (pvalue == NULL) {
			WIFI_ERR("%s: Null pvalue\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		slen = strlen(pvalue) + 1;
		if ((slen > MAX_STDSTR_LEN) ||
		    (remSpaceInStr(out_str, sizeof(out_str), pvalue) == FALSE) ||
		    (validStdsStr(out_str) == FALSE)) {
			pObj->reject_map |= Radio_OperatingStandards_MASK;
			WIFI_ERR("%s: Invalid std \n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		if (pObj->Radio.OperatingStandards) {
			WIFI_DBG("%s: free old OperatingStandards %s\n",
				__FUNCTION__, pObj->Radio.OperatingStandards);
			free(pObj->Radio.OperatingStandards);
		}
		pObj->Radio.OperatingStandards = malloc(slen);
		if (pObj->Radio.OperatingStandards == NULL) {
			WIFI_ERR("%s: malloc failed!\n", __FUNCTION__);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		WIFI_DBG("%s: Device.WiFi.%s.%d=[%s]\n",
			__FUNCTION__, parameter, radioIndex, pvalue);
		memcpy(pObj->Radio.OperatingStandards, (char *)(pvalue), slen);
		pObj->apply_map |= Radio_OperatingStandards_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];
		int ret;

		if (pvalue == NULL) {
			WIFI_ERR("%s: Null pvalue\n", __FUNCTION__);
			return -1;
		}

		slen = strlen(pvalue) + 1;
		if ((slen > MAX_STDSTR_LEN) ||
		    (remSpaceInStr(out_str, sizeof(out_str), pvalue) == FALSE) ||
		    (validStdsStr(out_str) == FALSE)) {
			WIFI_ERR("%s: Invalid std \n", __FUNCTION__);
			return -1;
		}

		snprintf(nvram_name, sizeof(nvram_name), "%s_oper_stands", nvifname);
		ret = wlcsm_nvram_set(nvram_name, pvalue);
		if (!ret) {
			WIFI_DBG("%s: CMD_SET_NVRAM %s=%s plen=%d\n",
				__FUNCTION__, nvram_name, pvalue, *plen);
		} else {
			WIFI_ERR("%s: setting %s=%s ret=%d plen=%d\n",
				__FUNCTION__, nvram_name, pvalue, ret, *plen);
		}
	}

	return 0;
}

/* Example usage:
 *  wldm_Radio_SupportedStandards(CMD_GET, radioIndex, supportedStandards, &len, NULL, NULL)
 */
int
wldm_Radio_SupportedStandards(int cmd, int radioIndex,
	list pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "SupportedStandards";
	char *osifname, *psupp_std, nvname[NVRAM_NAME_SIZE], caps[WLC_IOCTL_MEDLEN];
	int ret, len, nband, isax = 0;
	int list[4];

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	osifname = wldm_get_radio_osifname(radioIndex);
	if (cmd & CMD_GET) {
		ret = wl_iovar_get(osifname, "cap", (void *)caps, sizeof(caps));

		if (ret < 0) {
			WIFI_ERR("%s fail to get SupportedStandards ret=%d \r\n", __FUNCTION__, ret);
			return ret;
		} else {
			WIFI_DBG("%s caps = %s \n", __FUNCTION__, caps);
			if (strstr(caps, "11ax") != NULL)
				isax = 1;
		}
		/* WLC_GET_BANDLIST output: list[0] is count, followed by 'count' bands. */
		ret = wl_ioctl(osifname, WLC_GET_BANDLIST, list, sizeof(list));
		if (ret != 0) {
			WIFI_ERR("%s: WLC_GET_BANDLIST failed idx = %d\n", __FUNCTION__,
				radioIndex);
			return -1;
		}
		list[0] = dtoh32(list[0]);
		list[1] = dtoh32(list[1]);
		WIFI_DBG("%s isax=%d band=%d\n",  __FUNCTION__, isax, list[1]);
		if (list[0] == 1) {
			if (list[1] == WLC_BAND_2G)
				psupp_std = (isax == 0) ? WIFI_SUPPORTEDSTANDARDS_NONAX_2G :
					WIFI_SUPPORTEDSTANDARDS_AX_2G;
			else if (list[1] == WLC_BAND_5G)
				psupp_std = (isax == 0) ? WIFI_SUPPORTEDSTANDARDS_NONAX_5G :
					WIFI_SUPPORTEDSTANDARDS_AX_5G;
			else if (list[1] == WLC_BAND_6G)
				psupp_std = (isax == 0) ? WIFI_SUPPORTEDSTANDARDS_NONAX_6G :
					WIFI_SUPPORTEDSTANDARDS_AX_6G;
			else {
				WIFI_ERR("%s: invalid band = %d\n", __FUNCTION__, list[1]);
				return -1;
			}
		}
		else {
			WIFI_DBG("%d bands supported, check nband...\n", list[0]);
			snprintf(nvname, sizeof(nvname), "%s_nband",
				wldm_get_radio_nvifname(radioIndex));
			nband = atoi(nvram_safe_get(nvname));

			if (nband == WLC_BAND_2G || nband == WLC_BAND_AUTO)
				psupp_std = (isax == 0) ? WIFI_SUPPORTEDSTANDARDS_NONAX_2G :
					WIFI_SUPPORTEDSTANDARDS_AX_2G;
			else if (nband == WLC_BAND_5G)
				psupp_std = (isax == 0) ? WIFI_SUPPORTEDSTANDARDS_NONAX_5G :
					WIFI_SUPPORTEDSTANDARDS_AX_5G;
			else if (nband == WLC_BAND_6G)
				psupp_std = (isax == 0) ? WIFI_SUPPORTEDSTANDARDS_NONAX_6G :
					WIFI_SUPPORTEDSTANDARDS_AX_6G;
			else {
				WIFI_ERR("%s: invalid band = %d\n", __FUNCTION__, list[0]);
				return -1;
			}
		}
		len = strlen(psupp_std) + 1;
		if (*plen < len) {
			WIFI_ERR("%s: Buffer len=%d insufficient - need %d\n", __FUNCTION__,
				*plen, len);
			return -1;
		}
		*plen = len;
		memcpy(pvalue, psupp_std, *plen);
	}

	return 0;
}

/* Get the system uptime in secs.
 * cat /proc/uptime gives (e.g) 1559.22 2919.44
 *   - the uptime of the system (1559.22 seconds), and
 *   - the amount of time spent in idle process (2919.44 seconds)
 * we only use to the granularity of secs (1559) now - ignoring the rest
 */
static unsigned int
wl_get_systemUpSecs(void)
{
	char cmd[BUF_SIZE] = {0};
	FILE *fp = NULL;
	unsigned int upSecs = 0;
	int i = 0;

	snprintf(cmd, sizeof(cmd), "/bin/cat /proc/uptime > %s", WLDM_FILE_SYSTEM_UPTIME);
	if (system(cmd) == -1) {
		WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, cmd);
	}

	fp = fopen(WLDM_FILE_SYSTEM_UPTIME, "r");
	if (fp != NULL) {
		i = fscanf(fp, "%u", &upSecs);
		if (i == 0 || i == EOF) {
			WIFI_ERR("%s: Couldn't read from /proc/uptime\n", __FUNCTION__);
		}
		else {
			WIFI_DBG("%s upSecs=%u ......\n", __FUNCTION__, upSecs);
		}
		fclose(fp);
	}
	return upSecs;
}

/* Init or Get upTime for given radioIndex
 *  CMD_UPTIME_GET - gets the time the inteface has been up in secs
 *  CMD_UPTIME_INIT - initializes wlX.Y_boot_time in nvram to the system upTime
 */
int
wl_UpTime(int radioIndex, int cmd, unsigned int *upTime)
{
	char *nvifname, *nvram_value = NULL;
	char nvram_name[NVRAM_NAME_SIZE] = {0};
	unsigned int upSecs = wl_get_systemUpSecs();
	char buf[BUF_SIZE];

	nvifname = wldm_get_radio_nvifname(radioIndex);
	snprintf(nvram_name, sizeof(nvram_name), "%s_boot_time", nvifname);
	*upTime = 0;
	if (cmd == CMD_UPTIME_GET) {
		unsigned int startTime;

		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value != NULL) {
			startTime = (unsigned int)(atoi(nvram_value));
			WIFI_DBG("%s: startTime=%u upSecs=%u\n", __FUNCTION__, startTime, upSecs);
			if (upSecs > startTime) {
				*upTime = upSecs - startTime;
				return 0;
			} else {
				WIFI_ERR("%s: Err current startTime greater than systemUpSecs\n", __FUNCTION__);
			}
		}
		else {
			WIFI_ERR("%s: Err Null %s\n", __FUNCTION__, nvram_name);
		}
		/* if boot_time in nvram is not already initialized, do it now */
		cmd = CMD_UPTIME_INIT;
	}
	if (cmd == CMD_UPTIME_INIT) {
		snprintf(buf, sizeof(buf), "%u", upSecs);
		WIFI_DBG("%s Set %s=%s\n", __FUNCTION__, nvram_name, buf);
		if (wlcsm_nvram_set(nvram_name, buf) != 0) {
			WIFI_ERR("%s: Err saving %s to nvram\n",  __FUNCTION__, nvram_name);
			return -1;
		}
	}
	else {
		WIFI_ERR("%s Err unknown cmd=%d radioIndex=%d\n",  __FUNCTION__, cmd, radioIndex);
		return -1;
	}
	return 0;
}

int
wldm_Radio_LastChange(int cmd, unsigned int radioIndex,
	unsigned int *upSecs, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "LastChange";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_GET_NVRAM | CMD_SET_NVRAM);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET)) {
		*upSecs = 0;
		*plen = sizeof(*upSecs);
		if (wl_UpTime(radioIndex, CMD_UPTIME_GET, upSecs) < 0) {
			WIFI_ERR("%s Err getting radioUpTime\n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: upSecs=%d\n", __FUNCTION__, *upSecs);
	}
	return 0;
}
#define STA_ASSOC_COUNT					"/tmp/sta_assoc_count"

typedef struct link_bw {
	unsigned int					bw;
	char						*bw_str;
} link_bw_t;

static link_bw_t link_bw[] =				{{BW_20MHZ,	"20" },
							{ BW_40MHZ,	"40" },
							{ BW_80MHZ,	"80" },
							{ BW_160MHZ,	"160" }};

/* Function to find the total number of associations for the input mac */
static unsigned long long
find_assoc_count(int idx, int sub_idx, char *mac)
{
	int fidx, fsub_idx;
	unsigned long long assoc_count = 0, fassoc_count = 0;
	char fmac[ETHER_ADDR_STR_LEN];
	FILE *fp;

	fp = fopen(STA_ASSOC_COUNT, "r");
	if (!fp) {
		return 0;
	}
	while ((fscanf(fp, "%d %d %s %llu\n", &fidx, &fsub_idx, fmac, &fassoc_count) == 4)) {
		if ((idx == fidx) && (sub_idx == fsub_idx) && !strncasecmp(fmac, mac, sizeof(fmac))) {
			assoc_count = fassoc_count;
			break;
		}
	}
	fclose(fp);
	return assoc_count;
}

/* Function to call the ioctl for assoclist */
static int
wl_get_assoclist(char *osifname, char *ioctl_buf, uint buf_size)
{
	int ret;
	struct maclist *assoclist;
	int apidx;

	if ((apidx = wldm_get_apindex(osifname)) < 0) {
		WIFI_ERR("%s wrong apIndex for %s\n", __FUNCTION__, osifname);
		return -1;
	}
	memset(ioctl_buf, 0, buf_size);
	if (WLDM_AP_DISABLED(apidx)) return 0;
	assoclist = (struct maclist *) ioctl_buf;
	assoclist->count = htod32((buf_size - sizeof(int)) / ETHER_ADDR_LEN);
	ret = wl_ioctl(osifname, WLC_GET_ASSOCLIST, ioctl_buf, buf_size);
	if (ret < 0) {
		WIFI_ERR("Err: intf:%s assoclist\n", osifname);
		return -1;
	}
	return 0;
}

/* Function to call the ioctl for chanim_stats */
static int
wl_get_chanim_stats(char *osifname, uint stats_len, uint stats_count, char *ioctl_buf, uint buf_size)
{
	int ret = 0, buflen = 0;
	char *iptr;
	wl_chanim_stats_t chanim_stats;

	memset(&chanim_stats, 0, sizeof(chanim_stats));
	chanim_stats.buflen = htod32(stats_len);
	chanim_stats.count = stats_count;
	chanim_stats.version = WL_CHANIM_STATS_VERSION;
	memset(ioctl_buf, 0, buf_size);
	strcpy(ioctl_buf, "chanim_stats");
	buflen = strlen(ioctl_buf) + 1;
	iptr = (char *)(ioctl_buf + buflen);
	memcpy(iptr, &chanim_stats, sizeof(wl_chanim_stats_t));

	ret = wl_ioctl(osifname, WLC_GET_VAR, ioctl_buf, buf_size);
	if (ret < 0) {
		WIFI_ERR("Err: intf:%s chanim_stats\n", osifname);
		return -1;
	}
	return 0;
}

/* Function to call the ioctl for sta_info */
static int
wl_get_sta_info(char *osifname, struct ether_addr *ea, char *ioctl_buf, uint buf_size)
{
	int ret = 0, buflen = 0;
	char *iptr;

	memset(ioctl_buf, 0, buf_size);
	strcpy(ioctl_buf, "sta_info");
	buflen = strlen(ioctl_buf) + 1;
	iptr = (char *)(ioctl_buf + buflen);
	memcpy(iptr, ea, ETHER_ADDR_LEN);

	ret = wl_ioctl(osifname, WLC_GET_VAR, ioctl_buf, buf_size);
	if (ret < 0) {
		WIFI_ERR("Err: intf:%s sta_info\n", osifname);
		return -1;
	}
	return 0;
}

/* Function to call the ioctl for rssi */
static int
wl_get_rssi(char *osifname, struct ether_addr ea, int *rssi)
{
	scb_val_t scb_val;
	memset(&scb_val, 0, sizeof(scb_val));
	memcpy(&scb_val.ea, &ea, ETHER_ADDR_LEN);

	if (wl_ioctl(osifname, WLC_GET_RSSI, &scb_val, sizeof(scb_val)) < 0) {
		WIFI_ERR("Err:%s:%d RSSI %s fail\n", __FUNCTION__, __LINE__, osifname);
		return -1;
	}
	*rssi = dtoh32(scb_val.val);
	return 0;
}

/* Function to write the STA diagnostic info into wldm_wifi_associated_dev1/2/3_t
@param [in]  *plen - # of allocated wldm_wifi_associated_dev1/2/3_t struct
*/
static int
wl_getApAssociatedDeviceDiagnosticResult(int apIndex,
	void *associated_dev_struct_array, unsigned int *plen,
	wldm_diagnostic_result_t type)
{
	bool cli_AuthenticationState = 0, cli_Active = 1;
	char *nvifname, *osifname, ioctl_buf[MAX_IOCTL_BUFLEN], assoclist_buf[WLC_IOCTL_MEDLEN],
		*pcli_OperatingStandard, *pcli_OperatingChannelBandwidth = NULL,
		mac[ETHER_ADDR_STR_LEN];
	int i, idx, sub_idx = 0, cnt, noise, cli_SNR, cli_SignalStrength, cli_RSSI, band;
	unsigned int cli_LastDataDownlinkRate = 0, cli_LastDataUplinkRate = 0,
		cli_Retransmissions = 0;
	unsigned long cli_BytesSent = 0, cli_BytesReceived = 0, cli_PacketsSent = 0,
		cli_PacketsReceived = 0, cli_ErrorsSent = 0, cli_RetransCount = 0,
		cli_FailedRetransCount = 0, cli_RetryCount = 0, cli_DataFramesSentAck = 0,
		cli_DataFramesSentNoAck = 0;
	unsigned long long cli_Associations = 0;
	struct maclist *assoclist;
	sta_info_v8_t *sta_info_io;
	wl_chanim_stats_t *chan_list;
	chanim_stats_t *stats;
	wldm_wifi_associated_dev1_t *ptr1 = (wldm_wifi_associated_dev1_t *) associated_dev_struct_array;
	wldm_wifi_associated_dev2_t *ptr2 = (wldm_wifi_associated_dev2_t *) associated_dev_struct_array;
	wldm_wifi_associated_dev3_t *ptr3 = (wldm_wifi_associated_dev3_t *) associated_dev_struct_array;

	nvifname = wldm_get_nvifname(apIndex);
	sscanf(nvifname, "wl%d.%d", &idx, &sub_idx);
	osifname = wldm_get_osifname(apIndex);

	if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
		WIFI_ERR("%s: IOVAR Failed to get band info\n", __FUNCTION__);
		*plen = 0;
		return -1;
	}
	band = dtoh32(band);

	assoclist = (struct maclist *) assoclist_buf;
	if (wl_get_assoclist(osifname, assoclist_buf, sizeof(assoclist_buf)) < 0) {
		WIFI_ERR("Err: %s wl_get_assoclist error\n", osifname);
		*plen = 0;
		return -1;
	}
	*plen = (dtoh32(assoclist->count) < *plen) ? dtoh32(assoclist->count) : *plen;
	WIFI_DBG("%s: assoclist count = %d\n", __FUNCTION__, *plen);

	if (wl_get_chanim_stats(osifname, sizeof(wl_chanim_stats_t), 1, ioctl_buf, sizeof(ioctl_buf)) < 0) {
		WIFI_ERR("Err: %s wl_get_chanim error\n", osifname);
		*plen = 0;
		return -1;
	}
	chan_list = (wl_chanim_stats_t *) ioctl_buf;
	stats = chan_list->stats;
	noise = stats->bgnoise;

	for (cnt = 0; cnt < *plen; cnt++) {
		/* sta_info for each associated STA*/
		if (wl_get_sta_info(osifname, &assoclist->ea[cnt], ioctl_buf, sizeof(ioctl_buf)) < 0) {
			*plen = 0;
			return -1;
		}
		snprintf(mac, sizeof(mac), MACF, ETHER_TO_MACF(assoclist->ea[cnt]));
		sta_info_io = (sta_info_v8_t *)ioctl_buf;
		cli_AuthenticationState = (sta_info_io->flags & WL_STA_AUTHE) ? 1 : 0;
		pcli_OperatingStandard = (sta_info_io->flags & WL_STA_HE_CAP) ? "ax" :
			(sta_info_io->flags & WL_STA_VHT_CAP) ? "ac" :
			(sta_info_io->flags & WL_STA_N_CAP) ? "n" :
			(band == WLC_BAND_2G) ? "g" : "a";
		for (i = 0; i < (sizeof(link_bw) / sizeof(link_bw[0])); i++) {
			if (link_bw[i].bw == sta_info_io->link_bw) {
				pcli_OperatingChannelBandwidth = link_bw[i].bw_str;
				break;
			}
		}
		if (wl_get_rssi(osifname, assoclist->ea[cnt], &cli_RSSI) < 0) {
			WIFI_ERR("Err:%s:%d RSSI %s fail\n", __FUNCTION__, __LINE__, osifname);
			*plen = 0;
			return -1;
		}
		cli_SignalStrength = cli_RSSI;
		cli_SNR = cli_RSSI - noise;
		if (sta_info_io->flags & WL_STA_SCBSTATS) {
			cli_BytesSent = dtoh64(sta_info_io->tx_tot_bytes);
			cli_BytesReceived = dtoh64(sta_info_io->rx_tot_bytes);
			cli_DataFramesSentAck = dtoh32(sta_info_io->tx_pkts_total);
			cli_PacketsReceived = dtoh32(sta_info_io->rx_tot_pkts);
			cli_ErrorsSent = dtoh32(sta_info_io->tx_failures);
			cli_RetransCount = cli_DataFramesSentNoAck = dtoh32(sta_info_io->tx_pkts_retries);
			cli_FailedRetransCount = dtoh32(sta_info_io->tx_pkts_retry_exhausted);
			cli_RetryCount = dtoh32(sta_info_io->tx_pkts_retried);
			cli_PacketsSent = cli_DataFramesSentAck + cli_DataFramesSentNoAck;
		}
		cli_LastDataDownlinkRate = sta_info_io->tx_rate / 1000;
		cli_LastDataUplinkRate = sta_info_io->rx_rate / 1000;
		cli_Associations = find_assoc_count(idx, sub_idx, mac);
		cli_Retransmissions = sta_info_io->tx_pkts_retries;

		/* Fill up wldm_wifi_associated_dev1_t, wldm_wifi_associated_dev2_t and
			wldm_wifi_associated_dev3_t */
		memcpy(ptr1->cli_MACAddress, &assoclist->ea[cnt], ETHER_ADDR_LEN);
		ptr1->cli_AuthenticationState = cli_AuthenticationState;
		ptr1->cli_LastDataDownlinkRate = cli_LastDataDownlinkRate;
		ptr1->cli_LastDataUplinkRate = cli_LastDataUplinkRate;
		ptr1->cli_SignalStrength = cli_SignalStrength;
		ptr1->cli_Retransmissions = cli_Retransmissions;
		ptr1->cli_Active = cli_Active;
		strncpy(ptr1->cli_OperatingStandard, pcli_OperatingStandard,
			sizeof(ptr1->cli_OperatingStandard));
		strncpy(ptr1->cli_OperatingChannelBandwidth, pcli_OperatingChannelBandwidth,
			sizeof(ptr1->cli_OperatingChannelBandwidth));
		ptr1->cli_SNR = cli_SNR;
		ptr1->cli_BytesSent = cli_BytesSent;
		ptr1->cli_BytesReceived = cli_BytesReceived;
		ptr1->cli_RSSI = cli_RSSI;
		ptr1->cli_DataFramesSentAck = cli_DataFramesSentAck;
		ptr1->cli_DataFramesSentNoAck = cli_DataFramesSentNoAck;

		/* Fill up wldm_wifi_associated_dev2_t and wldm_wifi_associated_dev3_t */
		if (type >= DIAG_RESULT_2) {
			ptr2->cli_Associations = cli_Associations;
		}

		/* Fill up wldm_wifi_associated_dev3_t*/
		if (type >= DIAG_RESULT_3) {
#ifdef WL_TWT_LIST_VER
			struct ether_addr *peer = &(assoclist->ea[cnt]);
			char retbuf[WLC_IOCTL_SMLEN] = {0};
			wldm_twt_list_t *tlist = (wldm_twt_list_t *)retbuf;
			wldm_twt_params_t *tparamp = &(ptr3->cli_TwtParams.twtParams[0]);
			wldm_twt_sdesc_t *dp;
			int i, ret, alen;

			alen = sizeof(retbuf);
			ret = wl_getHE_TWTParams(apIndex, peer, (void *)(retbuf), &alen);
			if (ret < 0) {
				WIFI_ERR("%s apIndex[%d] Error wl_getHE_TWTParams\n",
					__FUNCTION__, apIndex);
				return -1;
			}
			if (alen > 0) {
				dp = &(tlist->desc[tlist->bcast_count]);
				ptr3->cli_TwtParams.numTwtSession = tlist->indv_count;
				for (i = 0; i < tlist->indv_count; i++, dp++, tparamp++) {
					wl_twt_sdesc_to_wldm_twt_params(dp, tparamp);
				}
			}
			else {
				ptr3->cli_TwtParams.numTwtSession = 0;
			}
#endif /* WL_TWT_LIST_VER */
			ptr3->cli_PacketsSent = cli_PacketsSent;
			ptr3->cli_PacketsReceived = cli_PacketsReceived;
			ptr3->cli_ErrorsSent = cli_ErrorsSent;
			ptr3->cli_RetransCount = cli_RetransCount;
			ptr3->cli_FailedRetransCount = cli_FailedRetransCount;
			ptr3->cli_RetryCount = cli_RetryCount;
		}
		if (type == DIAG_RESULT_1) {
			ptr1++;
		} else if (type == DIAG_RESULT_2) {
			ptr2++;
			ptr1 = (wldm_wifi_associated_dev1_t *)ptr2;
		} else {
			ptr3++;
			ptr2 = (wldm_wifi_associated_dev2_t *)ptr3;
			ptr1 = (wldm_wifi_associated_dev1_t *)ptr3;
		}
	}
	return 0;
}

/**********************************************
*  Device.WiFi.AccessPoint.{i}.AssociatedDevice
**********************************************/
int
wldm_AccessPoint_AssociatedDevice(int cmd, int apIndex, wldm_diagnostic_result_t type,
	void *associated_dev_array, unsigned int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "AssociatedDevice";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET | CMD_SET_NVRAM | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}
	if (cmd & CMD_GET) {
		if ((type != DIAG_RESULT_1) && (type != DIAG_RESULT_2) &&
			(type != DIAG_RESULT_3)) {
			WIFI_ERR("%d:%s Incorrect Request %d\n", __LINE__, __func__, type);
			return -1;
		}
		return wl_getApAssociatedDeviceDiagnosticResult(apIndex, associated_dev_array,
			plen, type);
	}
	return -1;
}

/* Plume mesh request SU-normalized average rx/tx rates */

/* some data struct and subtoutines */
struct wl_normal_sta_rate {
	float tried;
	float mbps_capacity; /* normalizes towards SU */
	float mbps_perceived; /* includes both SU+MU */
	float psr;
};

struct wl_normal_sta_pktq_stats {
	unsigned long long phyrate;
	unsigned long long acked;
	unsigned long long retry;
	unsigned long long bw;
	unsigned long long nss[4];
	unsigned long long tones;
	unsigned long long mumimo;
	unsigned long long muofdma;
};

int wl_normal_sta_get_tx_avg_rate_v6_mu(const wl_iov_pktq_log_t *resp,
	int i,
	struct wl_normal_sta_pktq_stats *stats)
{
#ifdef PKTQ_LOG_V06_HEADINGS_SIZE
	const mac_log_mu_counters_v06_t *c;
	int n;

	if (resp->version != 6)
		return -1;
	if ((resp->params.addr_type[i] & 0x7F) != 'M')
		return -1;

	n = resp->pktq_log.v06.num_prec[i];
	c = resp->pktq_log.v06.counters[i].mu;

	for (; n; n--, c++) {
		stats->mumimo += dtoh32(c->count[MAC_LOG_MU_VHTMU]);
		stats->mumimo += dtoh32(c->count[MAC_LOG_MU_HEMMU]);
		stats->mumimo += dtoh32(c->count[MAC_LOG_MU_HEMOM]);
		stats->muofdma += dtoh32(c->count[MAC_LOG_MU_HEMOM]);
		stats->muofdma += dtoh32(c->count[MAC_LOG_MU_HEOMU]);
		stats->tones += dtoh32(c->ru_count[MAC_LOG_MU_RU_26]) * 26;
		stats->tones += dtoh32(c->ru_count[MAC_LOG_MU_RU_52]) * 52;
		stats->tones += dtoh32(c->ru_count[MAC_LOG_MU_RU_106]) * 106;
		stats->tones += dtoh32(c->ru_count[MAC_LOG_MU_RU_242]) * 242;
		stats->tones += dtoh32(c->ru_count[MAC_LOG_MU_RU_484]) * 484;
		stats->tones += dtoh32(c->ru_count[MAC_LOG_MU_RU_996]) * 996;
		stats->tones += dtoh32(c->ru_count[MAC_LOG_MU_RU_2x996]) * 996 * 2;
	}

	WIFI_DBG("%s: %llu/%llu/%llu\n", __func__,
		stats->mumimo, stats->muofdma, stats->tones);
	return 0;
#else
	/* if it reports v6 but headers didn't say it is
	 * supported then somethimg is clearly wrong with the
	 * headers at build time and it needs to be addressed.
	 */
	if (resp->version == 6) {
		WIFI_WARNING("%s: version == 6\n", __func__);
	}
	return -1;
#endif
}

int wl_normal_sta_get_tx_avg_rate_v6(const wl_iov_pktq_log_t *resp,
	int i,
	struct wl_normal_sta_pktq_stats *stats)
{
#ifdef PKTQ_LOG_V06_HEADINGS_SIZE
	const pktq_log_counters_v06_t *c;
	int n;

	if (resp->version != 6)
		return -1;
	if ((resp->params.addr_type[i] & 0x7F) != 'A' &&
		(resp->params.addr_type[i] & 0x7F) != 'N')
		return -1;

	n = resp->pktq_log.v06.num_prec[i];
	c = resp->pktq_log.v06.counters[i].pktq;

	for (; n; n--, c++) {
		stats->phyrate += dtoh64(c->txrate_main) / 10;
		stats->acked += dtoh32(c->acked);
		stats->retry += dtoh32(c->retry);
		stats->bw += dtoh64(c->bandwidth);
		stats->nss[0] += dtoh32(c->nss[0]);
		stats->nss[1] += dtoh32(c->nss[1]);
		stats->nss[2] += dtoh32(c->nss[2]);
		stats->nss[3] += dtoh32(c->nss[3]);
	}

	WIFI_DBG("%s: %llu/%llu/%llu/%llu/%llu.%llu.%llu.%.llu\n", __func__,
		stats->phyrate, stats->acked, stats->retry, stats->bw, stats->nss[0],
		stats->nss[1], stats->nss[2], stats->nss[3]);
	return 0;
#else
	/* if it reports v6 but headers didn't say it is
	 * supported then somethimg is clearly wrong with the
	 * headers at build time and it needs to be addressed.
	 */
	if (resp->version == 6) {
		WIFI_WARNING("%s: version == 6\n", __func__);
	}

	return -1;
#endif
}

int wl_normal_sta_get_tx_avg_rate_v5(const wl_iov_pktq_log_t *resp,
	int i,
	struct wl_normal_sta_pktq_stats *stats)
{
	const pktq_log_counters_v05_t *c;
	int n;

	if (resp->version != 5)
		return -1;
	if ((resp->params.addr_type[i] & 0x7F) != 'A' &&
		(resp->params.addr_type[i] & 0x7F) != 'N')
		return -1;

	n = resp->pktq_log.v05.num_prec[i];
	c = resp->pktq_log.v05.counters[i];

	for (; n; n--, c++) {
		stats->phyrate += dtoh32(c->txrate_main) / 2;
		stats->acked += dtoh32(c->acked);
		stats->retry += dtoh32(c->retry);
	}

	WIFI_DBG("%s: %llu/%llu/%llu\n", __func__,
		stats->phyrate, stats->acked, stats->retry);
	return 0;
}

int wl_normal_sta_get_tx_avg_rate_v4(const wl_iov_pktq_log_t *resp,
	int i,
	struct wl_normal_sta_pktq_stats *stats)
{
	const pktq_log_counters_v04_t *c;
	int n;

	if (resp->version != 4)
		return -1;
	if ((resp->params.addr_type[i] & 0x7F) != 'A' &&
		(resp->params.addr_type[i] & 0x7F) != 'N')
		return -1;

	n = resp->pktq_log.v04.num_prec[i];
	c = resp->pktq_log.v04.counters[i];

	for (; n; n--, c++) {
		stats->phyrate += dtoh32(c->txrate_main) / 2;
		stats->acked += dtoh32(c->acked);
		stats->retry += dtoh32(c->retry);
	}

	WIFI_DBG("%s: %llu/%llu/%llu\n", __func__,
		stats->phyrate, stats->acked, stats->retry);
	return 0;
}

static inline unsigned int
wl_normal_sta_pktq_version(void)
{
#ifdef PKTQ_LOG_V06_HEADINGS_SIZE
	return 6;
#else
	return 4;
#endif
}

static inline unsigned long
wl_normal_sta_arr_sub(unsigned long long *arr, size_t arr_len, unsigned long long budget)
{
	unsigned long sub;
	size_t i;
	for (i = 0; i < arr_len; i++) {
		sub = arr[i] > budget ? budget : arr[i];
		arr[i] -= sub;
		budget -= sub;
	}
	return budget;
}

static int
wl_sta_get_tx_avg_rate(char *ifname, struct ether_addr *ea, int *tx_avg_rate)
{
	struct wl_normal_sta_rate sta_rate, *rate = &sta_rate;
	struct wl_normal_sta_pktq_stats stats = {0};
	wl_iov_mac_full_params_t req;
	wl_iov_pktq_log_t resp;
	unsigned long long sum_nss_su = 0;
	unsigned long long sum_nss;
	unsigned long max_nss;
	unsigned int pktq_ver = wl_normal_sta_pktq_version();
	unsigned int i;
	float mbps, avg_nss_su = 0, avg_nss = 0, nss_ratio = 0;
	float tones = 0, bw, bw_cnt_su, bw_cnt_mu, bw_avg, bw_ratio = 0;

	if (!ifname || !ea || !tx_avg_rate) {
		WIFI_ERR("%s: empty pointer\n", __FUNCTION__);
		return -1;
	}

	req.params.addr_type[0] = 'A';
	req.params.addr_type[1] = 'N';
	req.params.addr_type[2] = 'M';
	req.extra_params.addr_info[0] = 1 << 31; /* log auto bit, ie. all tids */
	req.extra_params.addr_info[1] = 1 << 31; /* log auto bit, ie. all tids */
	req.extra_params.addr_info[2] = 1 << 31; /* log auto bit, ie. all tids */

	memcpy(&req.params.ea[0], ea, sizeof(req.params.ea[0]));
	memcpy(&req.params.ea[1], ea, sizeof(req.params.ea[0]));
	memcpy(&req.params.ea[2], ea, sizeof(req.params.ea[0]));

	req.params.num_addrs = 2;
	req.params.num_addrs += pktq_ver >= 6 ? 1 : 0;
	req.params.num_addrs |= pktq_ver << 8;
	req.params.num_addrs = dtoh32(req.params.num_addrs);

	WIFI_DBG("%s: ifname=%s mac=%s pktq_stats addr=0x%08x\n", __FUNCTION__,
		 ifname, wl_ether_etoa(ea), dtoh32(req.params.num_addrs));

	if (wl_iovar_getbuf(ifname, "pktq_stats", &req,
		sizeof(wl_iov_mac_full_params_t), &resp, WLC_IOCTL_MAXLEN) < 0) {
		WIFI_ERR("Err to read pktq_stats: %s\n", ifname);
		return -1;
	}

	resp.version = dtoh32(resp.version);
	resp.params.num_addrs = dtoh32(resp.params.num_addrs);

	for (i = 0; i < resp.params.num_addrs; i++) {
		if (wl_normal_sta_get_tx_avg_rate_v6_mu(&resp, i, &stats) == 0)
			continue;
		if (wl_normal_sta_get_tx_avg_rate_v6(&resp, i, &stats) == 0)
			continue;
		if (wl_normal_sta_get_tx_avg_rate_v5(&resp, i, &stats) == 0)
			continue;
		if (wl_normal_sta_get_tx_avg_rate_v4(&resp, i, &stats) == 0)
			continue;
		WIFI_ERR("STrange version: %d\n", resp.version);
	}

	sum_nss = stats.nss[0] +
			  stats.nss[1] +
			  stats.nss[2] +
			  stats.nss[3];

	bw = stats.bw;
	bw /= sum_nss ?: stats.acked;

	if (stats.muofdma > 0) {
		tones = stats.tones;
		tones /= stats.muofdma;
	}

	if (sum_nss > stats.mumimo)
		sum_nss_su = sum_nss - stats.mumimo;

	max_nss = stats.nss[3] ? 4 :
			  stats.nss[2] ? 3 :
			  stats.nss[1] ? 2 :
			  stats.nss[0] ? 1 : 0;

	if (sum_nss > 0) {
		avg_nss = (1 * stats.nss[0]) +
				  (2 * stats.nss[1]) +
				  (3 * stats.nss[2]) +
				  (4 * stats.nss[3]);
		avg_nss /= sum_nss;
	}

	if (max_nss > 0)
		nss_ratio = avg_nss / max_nss;

	if (sum_nss_su > 0) {
		wl_normal_sta_arr_sub(stats.nss, ARRAY_SIZE(stats.nss), stats.mumimo);

		avg_nss_su = (1 * stats.nss[0]) +
					 (2 * stats.nss[1]) +
					 (3 * stats.nss[2]) +
					 (4 * stats.nss[3]);
		avg_nss_su /= sum_nss_su;
	}

	memset(rate, 0, sizeof(*rate));
	rate->tried = stats.acked + stats.retry;
	mbps = stats.phyrate;
	mbps /= rate->tried;

	if (rate->tried > 0) {
		/* The reported phyrate does not factor in MU-OFDMA
		 * tx RU reduction, but does implicitly have its nss
		 * constituent reduced by MU-MIMO tx. Therefore both
		 * capacity and perceived values need to be
		 * recovered.
		 *
		 * Capacity recovery assumes MU-MIMO will be
		 * exclusively responsible for nss
		 * degradation prioritizing nss=1 and up
		 * and therefore will result over-reported
		 * phyrates.
		 *
		 * Perceived recovery will under-report phyrates
		 * because RU tone do not scale linearly with BW.
		 */

		rate->mbps_capacity = mbps;
		if (max_nss > 0 && nss_ratio > 0 && avg_nss_su > 0) {
			rate->mbps_capacity /= nss_ratio;
			rate->mbps_capacity *= avg_nss_su;
			rate->mbps_capacity /= max_nss;
		}

		rate->mbps_perceived = mbps;
		if (bw > 0) {
			bw_cnt_su = bw * ((sum_nss ?: stats.acked) - stats.muofdma);
			bw_cnt_mu = tones * 0.078125 * 1.05 * stats.muofdma;
			bw_avg = (bw_cnt_su + bw_cnt_mu) / (sum_nss ?: stats.acked);
			bw_ratio = bw_avg / bw;
			rate->mbps_perceived *= bw_ratio;
		}

		rate->psr = stats.acked;
		rate->psr /= rate->tried;
	}

	WIFI_DBG("%s: ifname=%s mac=%s tones=%f bw=%f/%f nss=%lu/%f/%f tried=%f(%llu,%llu,%llu) mbps=%f/%f/%f psr=%f\n",
		  __FUNCTION__, ifname, wl_ether_etoa(ea),
		  tones, bw, bw_ratio,
		  max_nss, avg_nss, avg_nss_su,
		  rate->tried, stats.acked, stats.retry, sum_nss,
		  mbps, rate->mbps_capacity, rate->mbps_perceived,
		  rate->psr);

	*tx_avg_rate = (int)rate->mbps_capacity;
	WIFI_DBG("%s: Done tx_avg_rate=%d\n", __FUNCTION__, *tx_avg_rate);

	return 0;
}

static int
wl_sta_get_rx_avg_rate(char *ifname, struct ether_addr *ea, int *rx_avg_rate)
{
#ifdef SCB_RX_REPORT_DATA_STRUCT_VERSION
	int ret, flags = 0, i, tid, tid_count;
	union {
		iov_rx_report_struct_t cmd;
		char buf[WLC_IOCTL_MAXLEN];
	} resp;
	iov_rx_report_record_t *r;
#if SCB_RX_REPORT_DATA_STRUCT_VERSION >= 5
	uint8 *nrec;
#endif /* SCB_RX_REPORT_DATA_STRUCT_VERSION */
	struct wl_normal_sta_rate rate;
	float mbps, psr, phyrate, mpdu, ampdu, ampdu_ofdma, mpdu_ofdma;
	float phyrate_pkts = 0, phyrate_pkts_ofdma = 0;
	float bw, bw_cnt_su, bw_cnt_mu, bw_avg, bw_ratio, tones, retried;

	if (!ifname || !ea || !rx_avg_rate) {
		WIFI_ERR("%s: empty pointer\n", __FUNCTION__);
		return -1;
	}

	WIFI_DBG("Try rx_report: ifname=%s\n", ifname);

	ret = wl_iovar_getbuf(ifname, "rx_report", &flags, sizeof(flags),
		&resp, WLC_IOCTL_MAXLEN);
	if (ret < 0) {
		WIFI_ERR("Err to read rx_report: %s, err is %d\n", ifname, ret);
		return -1;
	}

#if SCB_RX_REPORT_DATA_STRUCT_VERSION >= 5
	WIFI_DBG("Got rx_report: version=%d count=%d record=%d (%d) counters=%d (%d)\n",
		resp.cmd.structure_version, resp.cmd.structure_count,
		resp.cmd.length_struct_record, sizeof(iov_rx_report_record_t),
		resp.cmd.length_struct_counters, sizeof(iov_rx_report_counters_t));
#else
	WIFI_DBG("Got rx_report: version=%d count=%d record=%d counters=%d\n",
		resp.cmd.structure_version, resp.cmd.structure_count,
		sizeof(iov_rx_report_record_t), sizeof(iov_rx_report_counters_t));
#endif /* SCB_RX_REPORT_DATA_STRUCT_VERSION */

	if (resp.cmd.structure_count == 0) {
		return -1;
	}

#if SCB_RX_REPORT_DATA_STRUCT_VERSION >= 5 /* should be 5 for offical */
	r = resp.cmd.structure_record;
	nrec = (uint8 *)r;
#endif /* SCB_RX_REPORT_DATA_STRUCT_VERSION */

	for (i = 0; i < resp.cmd.structure_count; i++) {
#if SCB_RX_REPORT_DATA_STRUCT_VERSION >= 5
		r = (iov_rx_report_record_t *)nrec;
#else
		r = &resp.cmd.structure_record[i];
#endif /* SCB_RX_REPORT_DATA_STRUCT_VERSION */
		WIFI_DBG("%s: i=%d r=%p (%p) sta_mac=%s rssi=%d(0x%x)\n", __FUNCTION__,
			i, r, &resp.cmd.structure_record[i],
			wl_ether_etoa(&(r->station_address)), r->rssi, r->rssi);

#if SCB_RX_REPORT_DATA_STRUCT_VERSION >= 5
		tid_count = bcm_bitcount((uint8*) &r->tid_report_mask, 1);

		nrec += (sizeof(iov_rx_report_record_t) +
			(tid_count - 1) * resp.cmd.length_struct_counters);

		WIFI_DBG("%s: tid_mask=0x%x, tid_count=%d delta=0x%x r=%p nrec=%p\n", __FUNCTION__,
			r->tid_report_mask, tid_count,(sizeof(iov_rx_report_record_t) +
			(tid_count - 1) * resp.cmd.length_struct_counters), r, nrec);
#else
		tid_count = bcm_bitcount((uint8*) &r->station_flags, 2);
		WIFI_DBG("%s: station_flags=0x%x, tid_count=%d\n", __FUNCTION__,
			r->station_flags, tid_count);
#endif /* SCB_RX_REPORT_DATA_STRUCT_VERSION */

		if (eacmp(&(r->station_address), ea) != 0) {
			WIFI_DBG("%s: i=%d MAC not match ea=%s\n", __FUNCTION__,
			i, wl_ether_etoa(ea));
			continue;
		}

		phyrate = 0;
		mpdu = 0;
		ampdu = 0;
		retried = 0;
		bw = 0;
		ampdu_ofdma = 0;
		mpdu_ofdma = 0;
		mbps = 0;
		psr = 0;
		bw_ratio = 1;
		tones = 0;

		memset(&rate, 0, sizeof(rate));

		for (tid = 0; tid < NUMPRIO && tid_count; tid++) {
#if SCB_RX_REPORT_DATA_STRUCT_VERSION >= 5
			if (!(r->tid_monitor_mask & (1 << tid))) {
				WIFI_DBG("%s: skip tid %d (mask=0x%x)\n",
					__FUNCTION__, tid, r->tid_monitor_mask);
				continue;
			}
#else
			if (!(r->station_flags & (1 << tid))) {
				WIFI_DBG("%s: skip tid %d (flags=0x%x)\n",
					__FUNCTION__, tid, r->station_flags);
				continue;
			}
#endif /* SCB_RX_REPORT_DATA_STRUCT_VERSION */

			WIFI_DBG("Got rx_report: i=%d tid=%d pcounters=0x%x phyrate=%d mpdu=%d ampdu=%d retried=%d bw=%d\n",
				i, tid, &(r->station_counters[tid]),
				r->station_counters[tid].rxphyrate,
				r->station_counters[tid].rxmpdu,
				r->station_counters[tid].rxampdu,
				r->station_counters[tid].rxretried,
				r->station_counters[tid].rxbw
				);

			phyrate += r->station_counters[tid].rxphyrate;
			mpdu += r->station_counters[tid].rxmpdu;
			ampdu += r->station_counters[tid].rxampdu;
			retried += r->station_counters[tid].rxretried;
			bw += r->station_counters[tid].rxbw;
#if SCB_RX_REPORT_DATA_STRUCT_VERSION == 2
			ampdu_ofdma += r->station_counters[tid].rxampdu_ofdma;
#endif
#if SCB_RX_REPORT_DATA_STRUCT_VERSION == 3
			mpdu_ofdma += r->station_counters[tid].rxmpdu_ofdma;
#endif
#if SCB_RX_REPORT_DATA_STRUCT_VERSION >= 2
			tones += r->station_counters[tid].rxtones;
#endif
		} /* for loop for tid */

		if (mpdu > 0) {
			psr = mpdu / (mpdu + retried);
		}

		phyrate_pkts = ampdu;
		phyrate_pkts_ofdma = ampdu_ofdma;

#if SCB_RX_REPORT_DATA_STRUCT_VERSION >= 3
		/* The rxphyrate accumulation was switched over from
		 * per-ampdu to per-mpdu in v3.
		 */
		phyrate_pkts = mpdu;
		phyrate_pkts_ofdma = mpdu_ofdma;
#endif

		if (phyrate_pkts > 0) {
			mbps = phyrate;
			mbps /= 1000;
			mbps /= phyrate_pkts;
			tones /= phyrate_pkts_ofdma ?: 1;
			bw /= phyrate_pkts;

			if (bw > 0) {
			   /* Reported phyrate is decreased by MU RU rx
				* so the SU capacity needs to be recovered.
				* It's not perfect because RU tone count
				* does not scale linearly with bandwidth.
				* The 5% is to roughly account for that.
				*/

				bw_cnt_su = bw * (phyrate_pkts - phyrate_pkts_ofdma);
				bw_cnt_mu = tones * 0.078125 * 1.05 * phyrate_pkts_ofdma;
				bw_avg = (bw_cnt_su + bw_cnt_mu) / phyrate_pkts;
				bw_ratio = bw_avg / bw;
			}
		}

		rate.tried = mpdu + retried;
		rate.mbps_capacity = mbps / bw_ratio;
		rate.mbps_perceived = mbps;
		rate.psr = psr;

		WIFI_DBG("%s: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx: "
			 "bw=%f/%f tones=%f ampdu=%f/%f mpdu=%f/%f tried=%f mbps=%f/%f psr=%f\n",
			 ifname,
			 r->station_address.octet[0],
			 r->station_address.octet[1],
			 r->station_address.octet[2],
			 r->station_address.octet[3],
			 r->station_address.octet[4],
			 r->station_address.octet[5],
			 bw, bw_ratio,
			 tones,
			 ampdu_ofdma, ampdu,
			 mpdu_ofdma, mpdu,
			 rate.tried,
			 rate.mbps_capacity,
			 rate.mbps_perceived,
			 rate.psr);

		*rx_avg_rate = (int)rate.mbps_capacity;
		WIFI_DBG("%s: Done rx_avg_rate=%d (%f %f)\n", __FUNCTION__,
			*rx_avg_rate, rate.mbps_capacity, rate.mbps_perceived);
		return 0;
	} /* per STA */

	WIFI_DBG("%s: No record for %s\n", __FUNCTION__, wl_ether_etoa(ea));
	return -1;
#else
	WIFI_ERR("%s: rx_report not support\n", __FUNCTION__);
	return -1;
#endif /* SCB_RX_REPORT_DATA_STRUCT_VERSION */
}

static int
dm_rate(int cmd, int apIndex, void *pvalue, int *plen, char *pvar)
{
	char *osifname, ioctl_buf[MAX_IOCTL_BUFLEN];
	sta_info_v4_t *sta_info;
	unsigned char ea[ETHER_ADDR_LEN];
	int rate = 0;

	if (cmd & CMD_GET) {
		if (!ether_atoe((char *)pvalue, ea)) {
			WIFI_ERR("%d:%s ether_atoe fail\n", __LINE__, __func__);
			return -1;
		}
		osifname = wldm_get_osifname(apIndex);
		if (wl_get_sta_info(osifname, (struct ether_addr *)&ea, ioctl_buf,
			sizeof(ioctl_buf)) < 0) {
			return -1;
		}
		sta_info = (sta_info_v4_t *)ioctl_buf;
		if (strcmp(pvar, "tx_rate") == 0) {
			/* Change from Kbps to Mbps */
			rate = sta_info->tx_rate / 1000;
		} else if (strcmp(pvar, "rx_rate") == 0) {
			/* Change from Kbps to Mbps */
			rate = sta_info->rx_rate / 1000;
		}
		memcpy(pvalue, &rate, *plen);
		return 0;
	}
	return -1;
}

static int
dm_assoc_dev_stats(int cmd, int apIndex, void *pvalue, int *plen, char *pvar)
{
	char *nvifname, *osifname, ioctl_buf[MAX_IOCTL_BUFLEN], *ptr = pvalue;
	sta_info_v4_t *sta_info;
	int i, idx, sub_idx = 0, avg_rate;
	unsigned char ea[ETHER_ADDR_LEN];
	wldm_wifi_associated_dev_stats_t assoc_dev_stats;
	unsigned long long associations;

	if (cmd & CMD_GET) {
		if (*plen < (sizeof(assoc_dev_stats) + sizeof(associations))) {
			WIFI_ERR("%d:%s Insufficient length: %d\n", __LINE__, __func__, *plen);
			return -1;
		}
		if (!ether_atoe((char *)pvalue, ea)) {
			WIFI_ERR("%d:%s ether_atoe fail\n", __LINE__, __func__);
			return -1;
		}
		nvifname = wldm_get_nvifname(apIndex);
		osifname = wldm_get_osifname(apIndex);
		if (wl_get_sta_info(osifname, (struct ether_addr *)&ea, ioctl_buf,
			sizeof(ioctl_buf)) < 0) {
			return -1;
		}
		sta_info = (sta_info_v4_t *)ioctl_buf;
		assoc_dev_stats.cli_rx_bytes = dtoh64(sta_info->rx_tot_bytes);
		assoc_dev_stats.cli_tx_bytes = dtoh64(sta_info->tx_tot_bytes);
		assoc_dev_stats.cli_rx_frames = dtoh32(sta_info->rx_tot_pkts);
		assoc_dev_stats.cli_tx_frames = dtoh32(sta_info->tx_tot_pkts);
		assoc_dev_stats.cli_rx_retries = dtoh32(sta_info->rx_pkts_retried);
		assoc_dev_stats.cli_tx_retries = dtoh32(sta_info->tx_pkts_retries);
		assoc_dev_stats.cli_rx_errors = dtoh32(sta_info->rx_decrypt_failures);
		assoc_dev_stats.cli_tx_errors = dtoh32(sta_info->tx_failures);
		if (wl_sta_get_rx_avg_rate(osifname, (struct ether_addr *)&ea, &avg_rate) == 0)
			assoc_dev_stats.cli_rx_rate = avg_rate;
		else
			assoc_dev_stats.cli_rx_rate = dtoh32(sta_info->rx_rate);
		if (wl_sta_get_tx_avg_rate(osifname, (struct ether_addr *)&ea, &avg_rate) == 0)
			assoc_dev_stats.cli_tx_rate = avg_rate;
		else
			assoc_dev_stats.cli_tx_rate = dtoh32(sta_info->tx_rate);
		for (i = WL_ANT_IDX_1; i < WL_STA_ANT_MAX; i++) {
			assoc_dev_stats.cli_rssi_bcn.rssi[i] =
					dtoh32(sta_info->rx_lastpkt_rssi[i]);
		}
		sscanf(nvifname, "wl%d.%d", &idx, &sub_idx);
		associations = find_assoc_count(idx, sub_idx, (char *)pvalue);
		memcpy(ptr, &assoc_dev_stats, sizeof(wldm_wifi_associated_dev_stats_t));
		ptr += sizeof(wldm_wifi_associated_dev_stats_t);
		memcpy(ptr, &associations, sizeof(associations));
	}
	return 0;
}

/* Function to call the iovar for rate_histo_report */
static int
wl_get_histo_report(char *osifname, uint8 ver, struct ether_addr ea, wl_rate_histo_report_t *rpt)
{
	wl_rate_histo_report_t param;

	memset(&param, 0, sizeof(wl_rate_histo_report_t));
	param.ver = ver;
	param.type = (ver == WL_HISTO_VER_1) ? WL_HISTO_TYPE_RATE_MAP1 : WL_HISTO_TYPE_RATE_MAP2;
	param.length = sizeof(wl_rate_histo_report_t) +
		(ver == WL_HISTO_VER_1) ? sizeof(wl_rate_histo_maps1_t) : sizeof(wl_rate_histo_maps2_t);
	param.fixed_len = WL_HISTO_VER_1_FIXED_LEN;
	memcpy(&param.ea, &ea, ETHER_ADDR_LEN);

	if (wl_iovar_getbuf(osifname, "rate_histo_report", &param, sizeof(wl_rate_histo_report_t),
		rpt, WLC_IOCTL_MAXLEN) < 0) {
		return -1;
	}
	return 0;
}

static int
dm_histo_report(int cmd, int apIndex, void *pvalue, int *plen, char *pvar)
{
	char *osifname, *p, macstr[ETHER_ADDR_STR_LEN],
		buf[WLC_IOCTL_MAXLEN];
	boolean rx_stats = FALSE;
	int rssi = 0, i, map_len;
	uint vht_index, array_count, rpt_len;
	unsigned long long associations;
	size_t maps_len;
	uint32 *ptr = NULL;
	wl_rate_histo_report_t *rpt = (wl_rate_histo_report_t *) buf;
	wl_rate_histo_maps1_t *maps1 = NULL;
	wl_rate_histo_maps2_t *maps2 = NULL;
	wl_rate_histo_map1_t *map1 = NULL;
	wl_rate_histo_map2_t *map2 = NULL;
	wldm_wifi_associatedDevRateInfoStats_t *pTmpStats = NULL;
	struct ether_addr ea;

	memcpy(macstr, pvalue, sizeof(macstr));
	*plen = 0;
	if (bcm_ether_atoe(macstr, &ea) == 0) {
		WIFI_ERR("Err:%s:%d bcm_ether_atoe fail\n", __FUNCTION__, __LINE__);
		return -1;
	}
	osifname = wldm_get_osifname(apIndex);
	if (strcmp(pvar, "rx_stats") == 0) {
		rx_stats = TRUE;
		if (wl_get_rssi(osifname, ea, &rssi) < 0) {
			WIFI_ERR("Err:%s:%d RSSI %s fail\n", __FUNCTION__, __LINE__, osifname);
		}
		if (rssi < 0) {
			rssi = rssi * -1;
		}
	}

	memset(buf, 0, sizeof(buf));
	if (wl_get_histo_report(osifname, WL_HISTO_VER_2, ea, rpt) < 0) {
		/* Try v1 if v2 fails */
		if (wl_get_histo_report(osifname, WL_HISTO_VER_1, ea, rpt) < 0) {
			WIFI_ERR("Err %s:%d rate_histo_report v1 and v2 failed\n", __FUNCTION__,
				__LINE__);
			return -1;
		}
	}

	rpt_len = sizeof(wl_rate_histo_report_t);

	if (rpt->ver == WL_HISTO_VER_1) {
		maps_len = sizeof(wl_rate_histo_maps1_t);
		map_len = WL_HISTO_MAP1_ARR_LEN;
		maps1 = WL_HISTO_DATA(rpt);
		map1 = rx_stats ? &maps1->rx : &maps1->tx;
		ptr = map1->arr;
	} else if (rpt->ver == WL_HISTO_VER_2) {
		maps_len = sizeof(wl_rate_histo_maps2_t);
		map_len = WL_HISTO_MAP2_ARR_LEN;
		maps2 = (wl_rate_histo_maps2_t *)WL_HISTO_DATA(rpt);
		map2 = rx_stats ? &maps2->rx : &maps2->tx;
		ptr = map2->arr;
	} else {
		WIFI_ERR("Err %s:%d Unsupported version %d\n", __FUNCTION__, __LINE__, rpt->ver);
		return -1;
	}
	rpt_len += maps_len;

	if ((rpt->type != WL_HISTO_TYPE_RATE_MAP1) && (rpt->type != WL_HISTO_TYPE_RATE_MAP2)) {
		WIFI_ERR("Err %s:%d Unsupported histogram type %d\n", __FUNCTION__, __LINE__,
			rpt->type);
		return -1;
	}

	if (rpt->length < rpt_len) {
		WIFI_ERR("Err %s:%d Report buffer length too short: %d < %d\n", __FUNCTION__,
			__LINE__, rpt->length, rpt_len);
		return -1;
	}

	if (rpt->length < (rpt->fixed_len + maps_len)) {
		WIFI_ERR("Err %s:%d Report map buffer length too short: %d < %d = %d+%d\n",
			__FUNCTION__, __LINE__, rpt->length, rpt->fixed_len + maps_len,
			rpt->fixed_len, maps_len);
		return -1;
	}

	memset(pvalue, 0, *plen);
	pTmpStats = (wldm_wifi_associatedDevRateInfoStats_t *)(pvalue);

	array_count = 0;
	for (i = 0; i < map_len; i++) {
		if (ptr[i] == 0) {
			continue;
		}
		pTmpStats[array_count].msdus = ptr[i];
		pTmpStats[array_count].mpdus = 0;
		if (rx_stats) {
			pTmpStats[array_count].rx_rssi_combined = rssi;
		}
		pTmpStats[array_count].flags = 2;
		if (i < WL_NUM_LEGACY_RATES) {
			pTmpStats[array_count].nss = 0;
			pTmpStats[array_count].mcs = 0;
			pTmpStats[array_count].bw = 20;

		} else {
			vht_index = (i - WL_NUM_LEGACY_RATES) % WL_NUM_VHT_RATES;
			pTmpStats[array_count].nss = WL_HISTO_RATE_DECODE_VHT_NSS(vht_index);
			pTmpStats[array_count].mcs = WL_HISTO_RATE_DECODE_VHT_MCS(vht_index);
			pTmpStats[array_count].bw = WL_HISTO_INDEX2BW(i);
		}
		array_count++;
	}
	*plen = array_count;
	p = (char *)pvalue;
	associations = find_assoc_count(apIndex, 0, macstr);
	p += array_count * sizeof(*pTmpStats);
	memcpy(p, &associations, sizeof(associations));
	return 0;
}

static xbrcm_t xbrcm_sta_tbl[] = {
	{  "rx_rate",			{dm_rate},			CMD_GET,		},
	{  "tx_rate",			{dm_rate},			CMD_GET,		},
	{  "assoc_dev_stats",		{dm_assoc_dev_stats},		CMD_GET,		},
	{  "rx_stats",			{dm_histo_report},		CMD_GET,		},
	{  "tx_stats",			{dm_histo_report},		CMD_GET,		},
	{  NULL,			{NULL},				0,			},
};

int
wldm_xbrcm_sta(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);
	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_sta_tbl);
}

/* unify the wifi callback mechanism */

typedef struct wldm_callback_info {
	int (*cb_handler)();
	char *domain_path;
	int cb_count;
} wldm_callback_info_t;

static wldm_callback_info_t wldm_callback_table[NUM_FD] = {
	{NULL, WIFI_HAL_CB_STA_CONN_DSOCKET, 0},
	{NULL, WIFI_HAL_CB_ASSOC_DEV_DSOCKET, 0},
	{NULL, WIFI_HAL_CB_AUTH_FAIL_DSOCKET, 0},
	{NULL, WIFI_HAL_CB_MESH_STEER_DSOCKET, 0},
	{NULL, WIFI_HAL_CB_RRM_BCNREP_DSOCKET, 0},
	{NULL, WIFI_HAL_CB_BSSTRANS_DSOCKET, 0},
	{NULL, WIFI_HAL_CB_DPP_DSOCKET, 0},
	{NULL, WIFI_HAL_CB_CH_CHG_DSOCKET, 0}
	};

static struct pollfd fds[NUM_FD] = {
	{.fd = -1},
	{.fd = -1},
	{.fd = -1},
	{.fd = -1},
	{.fd = -1},
	{.fd = -1},
	{.fd = -1},
	{.fd = -1}
	};

static nfds_t nfds = NUM_FD;

static int
wldm_domain_socket_init(int *sock_fd, char *sock_path)
{
	struct sockaddr_un sockaddr;
	int err = 0;

	if (sock_fd && *sock_fd < 0) {

		if (sock_path == NULL) {
			WIFI_ERR("%s@%d domain socket path is null\n", __FUNCTION__, __LINE__);
			return -1;
		}

		memset(&sockaddr, 0, sizeof(struct sockaddr_un));
		sockaddr.sun_family = AF_UNIX;

		snprintf(sockaddr.sun_path, UNIX_PATH_MAX, sock_path);
		if ((*sock_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
			WIFI_ERR("%s@%d Unable to create socket\n", __FUNCTION__, __LINE__);
			err = -1;
		} else if ((err = connect(*sock_fd, (struct sockaddr *)&sockaddr,
			sizeof(sockaddr))) < 0) {
			WIFI_ERR("%s@%d Unable to bind to loopback socket %d\n",
				__FUNCTION__, __LINE__, *sock_fd);
			err = -1;
		}

		if (err < 0 && *sock_fd >= 0) {
			WIFI_ERR("%s@%d: failure. Close socket\n", __FUNCTION__, __LINE__);
			close(*sock_fd);
			*sock_fd = -1;
		}
	}

	return err;
}

static void
wldm_domain_socket_close(int *sock_fd)
{
	WIFI_DBG("%s: *sock_fd=%d\n", __FUNCTION__, *sock_fd);
	if (*sock_fd >= 0) {
		close(*sock_fd);
		*sock_fd = -1;
	}
}

static void
wldm_callback_unregister(struct pollfd *pfd, wldm_callback_thread_t *ptr_cb_info)
{
	WIFI_DBG("%s: pfd->fd=%d\n", __FUNCTION__, pfd->fd);

	wldm_domain_socket_close(&pfd->fd);

	if (ptr_cb_info) {
		WIFI_DBG("%s: count=%d thread_initialized=%d\n",
			__FUNCTION__, ptr_cb_info->count, ptr_cb_info->thread_initialized);

		if (ptr_cb_info->count > 0)
			ptr_cb_info->count--;

		if ((ptr_cb_info->count == 0) && ptr_cb_info->thread_initialized) {
			pthread_cancel(ptr_cb_info->cbThreadId);
			ptr_cb_info->thread_initialized = 0;
		}
	}

	return;
}

/*	define a thread (listen to events from ecbd):
	- any event will trigger the corresponding callback handler;
	- callback handler will read the socket and invoke cb function (in wifi_hal)
*/

static void
wldm_thread_main_loop(void *arg)
{
	int ret = 0, i;
	struct pollfd *pfd;

	while (1) {
		ret = poll(fds, nfds, 3000);
		if (ret == -1) {
			if (errno == EINTR)
				continue;

			WIFI_DBG("poll failed - %s", strerror(errno));
			break;
		}

		/* use loop to browse all socket and call different callback handler if event in */
		for (i = 0; i < NUM_FD; i++) {
			pfd = &fds[i];
			if ((pfd->fd < 0) || !(pfd->revents & POLLIN) ||
				(wldm_callback_table[i].cb_handler == NULL)) {
				continue;
			}

			WIFI_DBG("%s: invoke callback handler for cb_id=%d fd=%d\n",
				__FUNCTION__, i, pfd->fd);
			ret = wldm_callback_table[i].cb_handler(pfd->fd);
			if (ret < 0) {
				if (ret == -EPIPE) {
					/* NOTE: The process that registered the callback will not
						be indicated about peer socket disconnection.
						No mechanism in place.
					*/
					WIFI_DBG("%s: Closing cb_id=%d fd=%d - Peer socket down\n",
						__FUNCTION__, i, pfd->fd);
					wldm_callback_unregister(pfd, NULL);
					wldm_callback_table[i].cb_handler = NULL;
				}
				WIFI_ERR("%s: Fail to call cb_handler for cb_id=%d with ret=%d\n",
					__FUNCTION__, i, ret);
			}
		}
	}
	pthread_exit(NULL);
}

static int
wldm_thread_create(wldm_callback_thread_t *ptr_cb_info)
{
	pthread_t cbThreadId;
	int ret = 0;

	if (pthread_create(&cbThreadId, NULL, (void *)&wldm_thread_main_loop, NULL)) {
		WIFI_ERR("Failed to spawn thread\n");
		pthread_cancel(cbThreadId);
		ret = -1;
	}
	else {
		WIFI_DBG("create thread success. cbThreadId=%lu\n", cbThreadId);
		ptr_cb_info->thread_initialized = 1;
		ptr_cb_info->cbThreadId = cbThreadId;
	}

	return ret;
}

static int
wldm_callback_register_init(struct pollfd *pfd, char *sock_path,
	wldm_callback_thread_t *ptr_cb_info)
{
	if (wldm_domain_socket_init(&pfd->fd, sock_path) == 0) {
		pfd->events = POLLIN;

		WIFI_DBG("create socket OK for %s, fd=%d, cb_thread_initialized=%d\n",
			sock_path, pfd->fd, ptr_cb_info->thread_initialized);
		if ((ptr_cb_info->thread_initialized == 0) &&
			(wldm_thread_create(ptr_cb_info) < 0)) {
			WIFI_ERR("%s Failed to create thread\n", __FUNCTION__);
			return -1;
		}
	}
	else {
		WIFI_ERR("Domain socket creation failed\n");
		return -1;
	}

	ptr_cb_info->count++;
	WIFI_DBG("%s: cb_total_count=%d for sock_path=%s\n",
		__FUNCTION__, ptr_cb_info->count, sock_path);
	return 0;
}

/*
	API for wifi_hal all callback register
	register or unregister callback handler
	reg = 1: register a callback handler
	reg = 0: unregister a callback handler
 */

int
wldm_callback(wldm_cb_action_t action, int cb_id,
	int (* cb_handler)(int), wldm_callback_thread_t *ptr_cb_info)
{
	struct pollfd *pfd;
	char *sock_path;

	WIFI_DBG("%s: action=%d cb_id=%d, ptr_cb_info=%p\n",
		__FUNCTION__, action, cb_id, ptr_cb_info);

	if (cb_id < 0 || cb_id >= NUM_FD) {
		WIFI_ERR("%s: Wrong cb_id %d\n", __FUNCTION__, cb_id);
		return -1;
	}

	if (ptr_cb_info == NULL) {
		WIFI_ERR("%s: Unexpected ptr_cb_info is NULL\n", __FUNCTION__);
		return -1;
	}

	pfd = &fds[cb_id];
	sock_path = wldm_callback_table[cb_id].domain_path;

	if (action == WLDM_CB_REGISTER) {
		/* register */
		if (pfd->fd >= 0) {
			WIFI_DBG("%s: pfd->fd=%d - already registered sock_path=%s\n",
				__FUNCTION__, pfd->fd, sock_path);
			return 0;
		}

		if (wldm_callback_register_init(pfd, sock_path, ptr_cb_info) == 0) {
			wldm_callback_table[cb_id].cb_handler = cb_handler;
			WIFI_DBG("%s: Succeed to register callback fd=%d path=%s cbThreadId=%lu\n",
				__FUNCTION__, pfd->fd, sock_path, ptr_cb_info->cbThreadId);
		}
		else {
			WIFI_ERR("%s: Fail to register callback for sock_path=%s\n",
				__FUNCTION__, sock_path);
			return -1;
		}
	}
	else if (action == WLDM_CB_UNREGISTER) {
		/* unregister */
		wldm_callback_unregister(pfd, ptr_cb_info);
		WIFI_DBG("%s: Unresigter cb_id %d count %d\n",
			__FUNCTION__, cb_id, ptr_cb_info->count);
	}
	else {
		WIFI_ERR("%s: Unknown callback action %d\n", __FUNCTION__, action);
		return -1;
	}
	return 0;
}
/* end of wldm_callback */

/* 802.11v BTM (BSS Transition Management) support:
	Move wlcsm_mngr_wifi_getBSSTransitionActivation() etc to wldm
*/

static int
wl_11v_SendBTMRequest(unsigned int apIndex, unsigned char peer_mac[6],
	void *in_request, uint option_len, char *ptr_option)
{
	int ret = 0;
	wl_af_params_t *afparamp;
	wl_action_frame_t *afInfo;
	dot11_bsstrans_req_t *transreq;
	char *osifname, *iovbufp = NULL, eabuf[ETHER_ADDR_STR_LEN], *ptr = (char *)in_request;
	uint iovblen, namelen;

	osifname = wldm_get_osifname(apIndex);

	WIFI_DBG("%s: apIndex=%d osifname=%s option_len=%d peer=%s\n",
		__FUNCTION__, apIndex, osifname, option_len, ether_etoa(peer_mac, eabuf));

	/* tx action frame */
	namelen = strlen("actframe") + 1;
	iovblen = sizeof(wl_af_params_t) + namelen;
	iovbufp = (char *)malloc(iovblen);
	if (iovbufp == NULL) {
		WIFI_ERR("%s: Fail to alloc buff [%d] \n", __FUNCTION__, iovblen);
		return -1;
	}

	memset(iovbufp, 0, iovblen);
	memcpy(iovbufp, "actframe", namelen);
	afparamp = (wl_af_params_t *)(iovbufp + namelen);
	afparamp->channel = 0;
	afparamp->dwell_time = -1;
	memset((&afparamp->BSSID), 0xFF, 6);

	/* action frame */
	afInfo = &afparamp->action_frame;
	bcopy(peer_mac, (afInfo->da.octet), 6);
	afInfo->packetId = (uint32)(uintptr)afInfo;
	afInfo->len = DOT11_BSSTRANS_REQ_LEN + option_len;

	/* BSS transition request */
	transreq = (dot11_bsstrans_req_t *)&afInfo->data[0];
	transreq->category = DOT11_ACTION_CAT_WNM; /* 10 */
	transreq->action = DOT11_WNM_ACTION_BSSTRANS_REQ; /* 07 */
	bcopy(ptr, (char*)&transreq->token, DOT11_BSSTRANS_REQ_LEN - 2); /* fixed len 7 */

	WIFI_DBG("%s: osifname=%s option_len=%d afInfo->len=%d\n",
		__FUNCTION__, osifname, option_len, afInfo->len);

	if (option_len) {
		ptr = ptr_option;
		WIFI_DBG("%s: in_request=%p ptr=%p p0=%x p1=%x\n",
			__FUNCTION__, in_request, ptr, ptr[0], ptr[1]);
		bcopy(ptr, transreq->data, option_len);
	}

	if (wl_ioctl(osifname, WLC_SET_VAR, iovbufp, iovblen) < 0) {
		WIFI_ERR("%s fail to send actframe ret=%d \r\n", __FUNCTION__, ret);
		ret = -1;
	} else {
		WIFI_DBG("wldm %s %s Sent BSSTRANS req actframe ret=%d iovblen=%d\n",
			__FUNCTION__, osifname, ret, iovblen);
		ret = 0;
	}

	free(iovbufp);
	return ret;
}

/* get/set: proprietary BTM setting on interface */
static int
wl_11v_BSSTransitionActivation(int cmd, unsigned int apIndex, bool *activatep)
{
	int ret = 0, val;
	char *osifname;

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_LIST | CMD_SET_NVRAM);

	osifname = wldm_get_osifname(apIndex);

	WIFI_DBG("%s: apIndex=%d osifname=%s cmd=%d activate=%d\n",
		__FUNCTION__, apIndex, osifname, cmd, *activatep);

	/* get first */
	if (wl_iovar_getint(osifname, "wnm", &val) < 0) {
		WIFI_ERR("%s fail to getBSSTransitionActivation ret=%d\n",
			__FUNCTION__, ret);
		return -1;
	}

	if (cmd & CMD_SET_IOCTL) {
		WIFI_DBG("%s: current wnm=0x%x, set to %d on %s\n",
			__FUNCTION__, val, *activatep, osifname);
		if (*activatep) {
			val |= WL_WNM_BSSTRANS;
		} else {
			val &= ~WL_WNM_BSSTRANS;
		}

		if (wl_iovar_set(osifname, "wnm", &val, sizeof(val)) < 0) {
			WIFI_ERR("%s fail to setBSSTransitionActivation ret=%d\n",
				__FUNCTION__, ret);
			return -1;
		}
	}

	if (cmd & CMD_GET) {
		*activatep = (val & WL_WNM_BSSTRANS) ? 1 : 0;
		WIFI_DBG("%s get wnm=0x%x activate=%d on %s\n",
			__FUNCTION__, val, *activatep, osifname);
	}

	return ret;
}

/* get for proprietary STA's extBTMCapability (wnm cap) */
static int
wl_11v_BTMClientCapability(unsigned int apIndex,
	unsigned char peer_mac[6], bool *extBTMCapability)
{
	struct ether_addr ea;
	sta_info_v7_t *sta;
	char *osifname, buf[WLC_IOCTL_MEDLEN] = {0}, eabuf[ETHER_ADDR_STR_LEN];
	int ret = 0, buflen = sizeof(buf);

	if (extBTMCapability == NULL) {
		WIFI_ERR("%s ptr is NULL\n", __FUNCTION__);
		return -1;
	}

	osifname = wldm_get_osifname(apIndex);

	memcpy(ea.octet, peer_mac, ETHER_ADDR_LEN);
	WIFI_DBG("%s: apIndex=%d osifname=%s peer_mac=%s\n",
		__FUNCTION__, apIndex, osifname, ether_etoa((unsigned char *)&ea, eabuf));

	if (wl_iovar_getbuf(osifname, "sta_info", (char*)&ea, ETHER_ADDR_LEN,
		buf, buflen) < 0) {
		WIFI_ERR("%s ioctl call for sta_info fail!\n", __FUNCTION__);
		return -1;
	}

	sta = (sta_info_v7_t *)buf;
	sta->ver = dtoh16(sta->ver);

	WIFI_DBG("%s: sta->ver=%d sta->wnm_cap=0x%x\n",
		__FUNCTION__, sta->ver, sta->wnm_cap);

	if (sta->ver > WL_STA_VER) {
		WIFI_ERR("%s: sta->ver:%d > %d!\n",
			__FUNCTION__, sta->ver, WL_STA_VER);
		return -1;
	}

	*extBTMCapability = (sta->wnm_cap & WL_WNM_BSSTRANS) ? 1 : 0;

	return ret;
}

static int
wl_11v_BSSTransitionImplemented(int cmd, unsigned int apIndex, bool *activatep)
{
	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_LIST | CMD_SET_IOCTL | CMD_SET_NVRAM);

	if (cmd & CMD_GET) {
		if (!activatep)
			return -1;
		*activatep = 1; /* dummy code */
		WIFI_DBG("%s get BTM implemented=1 on apIndex=%d\n",
			__FUNCTION__, apIndex);
	}

	return 0;
}

int
wldm_11v_btm(wldm_btm_action_t action, unsigned int apIndex, unsigned char peer_mac[6],
	bool *pvalue, char *in_request, unsigned int option_len, char *ptr_option)
{
	WIFI_DBG("%s: action=%d\n", __FUNCTION__, action);

	switch (action) {
		case WLDM_BTM_SEND_REQUEST:
			return wl_11v_SendBTMRequest(apIndex, peer_mac,
				in_request, option_len, ptr_option);

		case WLDM_BTM_GET_ACTIVATION:
			return wl_11v_BSSTransitionActivation(CMD_GET,
				apIndex, pvalue);

		case WLDM_BTM_SET_ACTIVATION:
			return wl_11v_BSSTransitionActivation(CMD_SET_IOCTL,
				apIndex, pvalue);

		case WLDM_BTM_GET_CLIENT_CAP:
			return wl_11v_BTMClientCapability(apIndex,
				peer_mac, pvalue);

		case WLDM_BTM_GET_IMPLEMENTED:
			return wl_11v_BSSTransitionImplemented(CMD_GET,
				apIndex, pvalue);

		default:
			WIFI_ERR("%s: action %d not support\n", __FUNCTION__, action);
			return -1;
	}
}

/* time unit (TU) is a unit of time equal to 1024 microseconds */
#define WL_AF_DWELL_TIME	100
/* for DPP support etc. */
int
wl_sendActionFrame(int apIndex, unsigned char peer[6], unsigned char bssid[6],
	uint frequency, char *frame, uint frame_len)
{
	int ret = 0;
	wl_af_params_t *afparamp;
	wl_action_frame_t *afInfo;
	uint iovblen, namelen;
	char *osifname = NULL, eabuf[ETHER_ADDR_STR_LEN], *iovbufp = NULL, *iovname = "actframe";

	osifname = wldm_get_osifname(apIndex);

	if (osifname == NULL) {
		WIFI_ERR("%s: osifname is NULL for apIndex=%d frequency=%d\n",
			__FUNCTION__, apIndex, frequency);
		return -1;
	}

	WIFI_DBG("%s: apIndex=%d osifname=%s peer=%s frequency=%d frame_len=%d\n",
		__FUNCTION__, apIndex, osifname, ether_etoa((unsigned char *)peer, eabuf),
		frequency, frame_len);

	/* tx action frame */

	namelen = strlen(iovname) + 1;
	iovblen = sizeof(wl_af_params_t) + namelen;
	iovbufp = (char *)malloc(iovblen);

	if (iovbufp == NULL) {
		WIFI_ERR("%s: Fail to alloc buff [%d] \n", __FUNCTION__, iovblen);
		return -1;
	}

	memset(iovbufp, 0, iovblen);

	memcpy(iovbufp, iovname, namelen);
	afparamp = (wl_af_params_t *)(iovbufp + namelen);
	afparamp->channel = frequency;
	afparamp->dwell_time = WL_AF_DWELL_TIME;
	if (bssid)
		bcopy(bssid, &(afparamp->BSSID), sizeof(afparamp->BSSID));
	else
		memset(&(afparamp->BSSID), 0xFF, sizeof(afparamp->BSSID));

	/* action frame */
	afInfo = &afparamp->action_frame;
	bcopy(peer, (afInfo->da.octet), sizeof(struct ether_addr));
	afInfo->packetId = (uint32)(uintptr)afInfo;
	afInfo->len = frame_len;
	bcopy(frame, afInfo->data, frame_len);

	ret = wl_ioctl(osifname, WLC_SET_VAR, (void *)iovbufp, (int)iovblen);

	if (ret < 0) {
		WIFI_ERR("%s: fail to send actframe ret=%d\n", __FUNCTION__, ret);
		ret = -1;
	} else {
		WIFI_DBG("%s: OK Sent actframe on %s ret=%d frame_len=%d iovblen=%d\n",
			__FUNCTION__, osifname, ret, frame_len, iovblen);
		ret = 0;
	}

	free(iovbufp);
	return ret;
}
/* end of wl_sendActionFrame (for DPP etc.) */

/* proprietary support for 802.11r (Fast BSS Transition, short for Fast Transition or FT) */
static int
wl_11r_ft_integer(int cmd, char *ftname, int apIndex, int *valuep)
{
	int ret = 0;
	char *osifname, *nvifname, buf[BUF_SIZE];

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_LIST);

	osifname = wldm_get_osifname(apIndex);
	nvifname = wldm_get_nvifname(apIndex);

	WIFI_DBG("%s: apIndex=%d osifname=%s cmd=0x%x ftname=%s value=%d\n",
		__FUNCTION__, apIndex, osifname, cmd, ftname, *valuep);

	if (cmd & CMD_GET) {
		if ((ret = wl_iovar_getint(osifname, ftname, valuep)) < 0) {
			WIFI_ERR("%s: fail to get %s ret=%d\n", __FUNCTION__, ftname, ret);
			return -1;
		}
		WIFI_DBG("%s: get %s=0x%x on %s\n",
			__FUNCTION__, ftname, *valuep, osifname);
	}

	if (cmd & CMD_GET_NVRAM) {
		NVRAM_INT_GET(nvifname, ftname, valuep);
		WIFI_DBG("%s: nvram get %s_%s=0x%x\n",
			__FUNCTION__, nvifname, ftname, *valuep);
	}

	if (cmd & CMD_SET_IOCTL) {
		WIFI_DBG("%s: set %s to %d on %s\n",
			__FUNCTION__, ftname, *valuep, osifname);

		if ((ret = wl_iovar_set(osifname, ftname, valuep, sizeof(*valuep))) < 0) {
			WIFI_ERR("%s: fail to set %s (%d) ret=%d (possible reason: "
				"wpa_auth must be WPA2 capable before enabling fbt)\n",
				__FUNCTION__, ftname, *valuep, ret);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		snprintf(buf, sizeof(buf), "%d", *valuep);
		NVRAM_STRING_SET(nvifname, ftname, buf);

	}

	return ret;
}

static int
wl_11r_ft_string(int cmd, char *ftname, int apIndex, char *id_str, int id_len)
{
	int ret = 0, is_r1kh_id = 0, len;
	char *osifname, *nvifname, buf[WLC_IOCTL_MEDLEN] = {0}, eabuf[ETHER_ADDR_STR_LEN];
	struct ether_addr ea;

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_LIST);

	osifname = wldm_get_osifname(apIndex);

	WIFI_DBG("%s: apIndex=%d osifname=%s cmd=0x%x ftname=%s id_str=%s id_len=%d\n",
		__FUNCTION__, apIndex, osifname, cmd, ftname, id_str, id_len);

	if ((ret = wl_iovar_get(osifname, ftname, buf, sizeof(buf))) < 0) {
		WIFI_ERR("%s: fail to get %s ret=%d\n", __FUNCTION__, ftname, ret);
		return -1;
	}

	if (strcmp(ftname, "fbt_r1kh_id") == 0) {
		is_r1kh_id = 1;
	}

	if (cmd & CMD_GET) {
		if (is_r1kh_id) {
			ether_etoa((unsigned char *)buf, id_str);
		}
		else {
			strncpy(id_str, buf, id_len);
			id_str[id_len] = 0;
		}
		WIFI_DBG("%s: get %s=%s id_len=%d on %s\n",
			__FUNCTION__, ftname, id_str, id_len, osifname);
	}

	if (cmd & CMD_SET_IOCTL) {
		len = strlen(id_str) > id_len ? id_len : strlen(id_str);

		WIFI_DBG("%s: current %s=%s, set to %s (len=%d) on %s\n",
			__FUNCTION__, ftname, buf, id_str, len, osifname);

		if (is_r1kh_id) {
			ether_atoe(id_str, &ea.octet[0]);
			WIFI_DBG("%s: For fbt_r1kh_id %s ID(mac)=%s\n",
				__FUNCTION__, ftname, ether_etoa((unsigned char *)&ea, eabuf));
			if ((ret = wl_iovar_set(osifname, ftname, &ea, sizeof(ea))) < 0) {
				WIFI_ERR("%s: fail to set %s to %s ret=%d\n",
					__FUNCTION__, ftname, id_str, ret);
				return -1;
			}
		}
		else {
			if ((ret = wl_iovar_set(osifname, ftname, id_str, len)) < 0) {
				WIFI_ERR("%s: fail to set %s to %s ret=%d\n",
					__FUNCTION__, ftname, id_str, ret);
				return -1;
			}
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		nvifname = wldm_get_nvifname(apIndex);
		NVRAM_STRING_SET(nvifname, ftname, id_str);
	}

	return ret;
}

int
wldm_11r_ft(wldm_ft_action_t action, int apIndex, void *pvalue, int *plen, char *pbuf)
{
	int val, len, radioIndex, ret = 0, up_cmd = 0;
	boolean is_up = FALSE, up_reqd = FALSE, enable;
	char *osifname = wldm_get_osifname(apIndex);

	WIFI_DBG("%s: action=%d\n", __FUNCTION__, action);

	if (plen && (*plen > sizeof(val))) {
		WIFI_ERR("%s: *plen=%d too long for action %d\n", __FUNCTION__, *plen, action);
		return -1;
	}

	radioIndex = wldm_get_radioIndex(apIndex);
	if (action == WLDM_FT_SET_ACTIVATED ||
		action == WLDM_FT_SET_OverDSACTIVATED ||
		action == WLDM_FT_SET_MobilityDomainID ||
		action == WLDM_FT_SET_R0KeyHolderID ||
		action == WLDM_FT_SET_R1KeyHolderID) {

		len = sizeof(boolean);

		wldm_Radio_Enable(CMD_GET, radioIndex, &is_up, &len, NULL, NULL);

		/* use is11AXCapable as flag for iovar IOVF_SET_DOWN/IOVF_SET_UP */
		if (is11AXCapable(osifname) == 0) {
			up_reqd = TRUE;
		}

		WIFI_DBG("%s: orig_status=%d request=%d\n", __FUNCTION__, is_up, up_reqd);

		if (is_up != up_reqd) {
			enable = up_reqd;
			up_cmd = 1;
			wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex, &enable, &len, NULL, NULL);
		}
	}

	switch (action) {
		case WLDM_FT_GET_ACTIVATED:
			/* wifi_hal use "bool" to get FT ACTIVATED */
			if (wl_11r_ft_integer(CMD_GET, "fbt", apIndex, &val) < 0) {
				WIFI_ERR("%s: get fbt err\n", __FUNCTION__);
				return -1;
			}

			memcpy(pvalue, &val, *plen);
			WIFI_DBG("%s: get fbt=%d val=%d len=%d\n", __FUNCTION__, *(bool *)pvalue, val, *plen);
			break;

		case WLDM_FT_GET_ACTIVATED_NVRAM:
			/* wifi_hal use "uchar" to set FT ACTIVATED */
			val = 0;
			ret = wl_11r_ft_integer(CMD_GET_NVRAM, "fbt", apIndex, &val);
			val = (val == 0) ? FALSE : TRUE;

			memcpy(pvalue, &val, *plen);
			break;

		case WLDM_FT_SET_ACTIVATED:
			/* wifi_hal use "uchar" to set FT ACTIVATED */
			val = 0;
			memcpy(&val, pvalue, *plen);
			WIFI_DBG("%s: set fbt=%d *pvalue=%d *plen=%d\n",
				__FUNCTION__, val, *(char *)pvalue, *plen);
			ret = wl_11r_ft_integer(CMD_SET_IOCTL, "fbt", apIndex, &val);
			break;

		case WLDM_FT_SET_ACTIVATED_NVRAM:
			/* wifi_hal use "uchar" to set FT ACTIVATED */
			val = 0;
			memcpy(&val, pvalue, *plen);
			WIFI_DBG("%s: set fbt=%d *pvalue=%d *plen=%d\n",
				__FUNCTION__, val, *(char *)pvalue, *plen);
			ret = wl_11r_ft_integer(CMD_SET_NVRAM, "fbt", apIndex, &val);
			break;

		case WLDM_FT_GET_OverDSACTIVATED:
			/* both get/set for OverDSACTIVATED are bool */
			if (wl_11r_ft_integer(CMD_GET, "fbtoverds", apIndex, &val) < 0) {
				WIFI_ERR("%s: get fbtoverds err\n", __FUNCTION__);
				return -1;
			}

			memset(pvalue, 0, *plen);
			memcpy(pvalue, &val, *plen);
			WIFI_DBG("%s: get fbtoverds=%d val=%d len=%d\n", __FUNCTION__, *(bool *)pvalue, val, *plen);
			break;

		case WLDM_FT_SET_OverDSACTIVATED:
			val = 0;
			memcpy(&val, pvalue, *plen);
			WIFI_DBG("%s: set fbtoverds=%d *pvalue=%d *plen=%d\n", __FUNCTION__, val, *(bool *)pvalue, *plen);
			ret = wl_11r_ft_integer(CMD_SET_IOCTL | CMD_SET_NVRAM, "fbtoverds",
				apIndex, &val);
			break;

		case WLDM_FT_GET_MobilityDomainID:
			/* wifi_hal use UCHAR[2], its actual type is "short" */
			if (wl_11r_ft_integer(CMD_GET, "fbt_mdid", apIndex, &val) < 0) {
				WIFI_ERR("%s: get fbt_mdid err\n", __FUNCTION__);
				return -1;
			}

			memcpy(pbuf, &val, *plen);
			WIFI_DBG("%s: get fbt_mdid=%d (%d)\n", __FUNCTION__, val, *(unsigned short *)pbuf);

			break;

		case WLDM_FT_SET_MobilityDomainID:
			/* wifi_hal use UCHAR[2], convert to its actual type "short" */
			val = 0;
			memcpy(&val, pbuf, *plen);
			WIFI_DBG("%s: set fbt_mdid=%d *plen=%d\n", __FUNCTION__, val, *plen);
			ret = wl_11r_ft_integer(CMD_SET_IOCTL | CMD_SET_NVRAM, "fbt_mdid",
				apIndex, &val);
			break;

		case WLDM_FT_GET_R0KeyHolderID:
			return wl_11r_ft_string(CMD_GET, "fbt_r0kh_id", apIndex, pbuf, FT_LEN_KHID);

		case WLDM_FT_SET_R0KeyHolderID:
			ret = wl_11r_ft_string(CMD_SET_IOCTL | CMD_SET_NVRAM, "fbt_r0kh_id",
				apIndex, pbuf, FT_LEN_KHID);
			break;

		case WLDM_FT_GET_R1KeyHolderID:
			return wl_11r_ft_string(CMD_GET, "fbt_r1kh_id", apIndex, pbuf, FT_LEN_KHID);

		case WLDM_FT_SET_R1KeyHolderID:
			/* pbuf is MAC format string */
			ret = wl_11r_ft_string(CMD_SET_IOCTL | CMD_SET_NVRAM, "fbt_r1kh_id",
				apIndex, pbuf, FT_LEN_KHID);
			break;

		default:
			WIFI_ERR("%s: action %d not support\n", __FUNCTION__, action);
			return -1;
	}

	if (up_cmd) {
		/* restore to the orginal status */
		enable = is_up;
		WIFI_DBG("%s: Restore orig_status=%d request=%d\n", __FUNCTION__, is_up, up_reqd);
		wldm_Radio_Enable(CMD_SET_IOCTL, radioIndex, &enable, &len, NULL, NULL);
	}

	return ret;
}
/* end of 802.11r */

/* WFA WPS */
static int
set_wps_env(char *uibuf)
{
	int wps_fd = -1, sentBytes = 0;
	struct sockaddr_in to;
	uint32 uilen = strlen(uibuf);

	if ((wps_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		goto exit;
	}

	/* send to wps_monitor */
	to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	to.sin_family = AF_INET;
	to.sin_port = htons(WPS_UI_PORT);

	sentBytes = sendto(wps_fd, uibuf, uilen, 0, (struct sockaddr *) &to,
		sizeof(struct sockaddr_in));

	if (sentBytes != uilen) {
		goto exit;
	}

	/* Sleep 100 ms to make sure wps_monitor have received socket */
	usleep(100*1000);
	close(wps_fd);
	return 0;

exit:
	if (wps_fd >= 0)
		close(wps_fd);

	return -1;
}

static int
wldm_WFA_WPS_Cancel(int apIndex)
{
	char *nvifname = wldm_get_nvifname(apIndex);
	char *osifname = wldm_get_osifname(apIndex);
	char nv_name[128], cmd[128];

	WIFI_DBG("%s: apIndex=%d\n", __FUNCTION__, apIndex);

	snprintf(nv_name, sizeof(nv_name), "%s_wps_mode", nvifname);
	if (!nvram_match(nv_name, "enabled")) {
		WIFI_ERR("%s: wps is disabled on %s\n", __FUNCTION__, nvifname);
		return -1;
	}

	if (!HAPD_DISABLED()) {

		snprintf(cmd, sizeof(cmd), "hostapd_cli -p"
			" %s -i %s wps_cancel", HAPD_DIR, osifname);
		WIFI_DBG("%s: %s\n", __FUNCTION__, cmd);
		if (system(cmd) == 0) {
			wl_wlif_update_wps_ui(WLIF_WPS_UI_INIT);
		}
		else {
			WIFI_ERR("%s: failed\n", __FUNCTION__);
			return -1;
		}
	}
	else {
		snprintf(cmd, sizeof(cmd), "SET wps_config_command=%d wps_action=%d",
			WPS_UI_CMD_STOP, WPS_UI_ACT_NONE);
		WIFI_DBG("%s: %s\n", __FUNCTION__, cmd);
		set_wps_env(cmd);
	}

	return 0;
}

static int
wldm_WFA_WPS_ActivatePushButton(int apIndex)
{
	char buf[BUF_SIZE];
	char *nv_value = NULL, *osifname = NULL;

	WIFI_DBG("%s: apIndex=%d\n", __FUNCTION__, apIndex);
	wldm_WFA_WPS_Cancel(apIndex);

	if (!HAPD_DISABLED()) {
		/* Send wps_pbc command to hostapd */
		char nv_name[NVRAM_NAME_SIZE];
		char *nvifname = wldm_get_nvifname(apIndex);

		snprintf(nv_name, sizeof(nv_name), "%s_wps_mode", nvifname);
		nv_value = wlcsm_nvram_get(nv_name);
		if (!nv_value || (strcmp(nv_value, "enabled") != 0)) {
			WIFI_ERR("%s: wps is disabled on %s\n", __FUNCTION__, nvifname);
			return -1;
		}

		osifname = wldm_get_osifname(apIndex);
		snprintf(buf, sizeof(buf), "hostapd_cli -p"
					" %s -i %s wps_pbc", HAPD_DIR, osifname);
		WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
		if (system(buf) == 0)
			wl_wlif_update_wps_ui(WLIF_WPS_UI_FINDING_PBC_STA);
		else
			WIFI_ERR("%s: fail to run '%s'\n", __FUNCTION__, buf);
	}
	else {
		/* Send command to wps_monitor */
		int len = 0;
		int radioIndex = 0;

		radioIndex = wldm_get_radioIndex(apIndex);
		osifname = wldm_get_radio_osifname(radioIndex);
		/* Send command to wps_monitor */
		len += snprintf(buf + len, sizeof(buf) - len,
			"SET wps_method=%d "
			"wps_sta_pin=00000000 "
			"wps_action=%d "
			"wps_config_command=%d "
			"wps_pbc_method=%d "
			"wps_ifname=%s",
			WPS_UI_METHOD_PBC,
			WPS_UI_ACT_ADDENROLLEE,
			WPS_UI_CMD_START,
			WPS_UI_PBC_SW,
			osifname);

		nv_value  = wlcsm_nvram_get("wps_version2");
		if (nv_value && !strcmp(nv_value, "enabled")) {
			char autho_sta_mac[32];

			memset(autho_sta_mac, '\0', sizeof(autho_sta_mac));
			nv_value = wlcsm_nvram_get("wps_autho_sta_mac");
			if (nv_value)
				strncpy(autho_sta_mac, nv_value, sizeof(autho_sta_mac) - 1);
			len += snprintf(buf + len, sizeof(buf) - len,
					" wps_autho_sta_mac=%s", autho_sta_mac);
		}

		WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
		if (set_wps_env(buf)) {
			WIFI_ERR("%s: fail to send command to wps_monitor\n", __FUNCTION__);
			return -1;
		}
	}

	wlcsm_nvram_set("wps_config", "DONE");
	return 0;
}

static int
wldm_WFA_WPS_SetClientPin(int apIndex, string pin)
{
	char *nv_value = NULL, *osifname = NULL;
	char buf[BUF_SIZE];

	WIFI_DBG("%s: apIndex=%d\n", __FUNCTION__, apIndex);
	wldm_WFA_WPS_Cancel(apIndex);

	if (!HAPD_DISABLED()) {
		/* Send wps_pbc command to hostapd */
		char nv_name[NVRAM_NAME_SIZE];
		char buf[BUF_SIZE];
		char autho_sta_mac[32];
		char *nvifname = wldm_get_nvifname(apIndex);

		memset(autho_sta_mac, '\0', sizeof(autho_sta_mac));
		nv_value = wlcsm_nvram_get("wps_autho_sta_mac");
		if (nv_value)
			strncpy(autho_sta_mac, nv_value, sizeof(autho_sta_mac) - 1);

		snprintf(nv_name, sizeof(nv_name), "%s_wps_mode", nvifname);
		nv_value = wlcsm_nvram_get(nv_name);
		if (!nv_value || (strcmp(nv_value, "enabled") != 0)) {
			WIFI_ERR("%s: wps is disabled on %s\n", __FUNCTION__, nvifname);
			return -1;
		}

		osifname = wldm_get_osifname(apIndex);
		snprintf(buf, sizeof(buf),
			"hostapd_cli -p %s -i %s wps_pin any %s %s",
			HAPD_DIR, osifname, pin, autho_sta_mac);
		WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
		if (system(buf) == 0)
			wl_wlif_update_wps_ui(WLIF_WPS_UI_FINDING_PBC_STA);
		else
			WIFI_ERR("%s: fail to run '%s'\n", __FUNCTION__, buf);
	} else {
		/* Send command to wps_monitor */
		int len = 0;
		int radioIndex = 0;

		radioIndex = wldm_get_radioIndex(apIndex);
		osifname = wldm_get_radio_osifname(radioIndex);
		len += snprintf(buf + len, sizeof(buf) - len,
				"SET wps_method=%d "
				"wps_sta_pin=%s "
				"wps_action=%d "
				"wps_config_command=%d "
				"wps_pbc_method=%d "
				"wps_ifname=%s",
				WPS_UI_METHOD_PIN,
				pin,
				WPS_UI_ACT_ADDENROLLEE,
				WPS_UI_CMD_START,
				WPS_UI_PBC_SW,
				osifname);

		nv_value  = wlcsm_nvram_get("wps_version2");
		if (nv_value && !strcmp(nv_value, "enabled")) {
			char autho_sta_mac[32];

			memset(autho_sta_mac, '\0', sizeof(autho_sta_mac));
			nv_value = wlcsm_nvram_get("wps_autho_sta_mac");
			if (nv_value)
				strncpy(autho_sta_mac, nv_value, sizeof(autho_sta_mac) - 1);
			len += snprintf(buf + len, sizeof(buf) - len,
					" wps_autho_sta_mac=%s", autho_sta_mac);
		}

		WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
		if (set_wps_env(buf)) {
			WIFI_ERR("%s fail to send command to wps_monitor\n", __FUNCTION__);
			return -1;
		}
	}

	wlcsm_nvram_set("wps_config", "DONE");
	return 0;
}

static int
wldm_WFA_WPS_GetStatus(int apIndex, char *buf, int bufLen)
{
	int iStatus;
	char *status = "Idle";

	if (!buf || !bufLen) {
		WIFI_ERR("%s: %d: null buf\n", __FUNCTION__, apIndex);
		return -1;
	}

	if (!HAPD_DISABLED()) {
		char cmd[128], output[256], *pStr;
		char *osifname = NULL;

		osifname = wldm_get_osifname(apIndex);
		if (!osifname) {
			WIFI_ERR("%s: %d: Fail to get osifname\n", __FUNCTION__, apIndex);
			return -1;
		}

		//Query wps status from hostapd via "hostapd_cli get_wps_status"
		snprintf(cmd, sizeof(cmd), "hostapd_cli -i %s wps_get_status",
				osifname);
		if (syscmd(cmd, output, sizeof(output)) != 0) {
			WIFI_ERR("%s: %d: \"%s\" failed\n", __FUNCTION__, apIndex, cmd);
			return -1;
		}

		//parse the output buffer to get wps status
		//TODO: WPS PIN
		WIFI_ERR("%s: %d: wps_status: %s\n", __FUNCTION__, apIndex, output);
		pStr = strstr(output, "PBC Status: ");
		if (!pStr)
			goto PARSE_ERR;
		pStr += strlen("PBC Status: ");
		if (!strncmp(pStr, "Active", strlen("Active"))) {
			status = "In_progress";
		} else if (!strncmp(pStr, "Overlap", strlen("Overlap"))) {
			status = "Failed";
		} else {
			pStr = strstr(output, "Last WPS result: ");
			if (!pStr)
				goto PARSE_ERR;
			pStr += strlen("Last WPS result: ");
			if (!strncmp(pStr, "Success", strlen("Success")))
				status = "Success";
			else if (!strncmp(pStr, "Failed", strlen("Failed")))
				status = "Failed";
			else if (!strncmp(pStr, "None", strlen("None")))
				status = "Idle";
			else
				goto PARSE_ERR;
		}
		snprintf(buf, bufLen, status);
		return 0;

PARSE_ERR:
		WIFI_ERR("%s: %d: Fail to parse output of \"%s\"\n", __FUNCTION__,
			apIndex, cmd);
		return -1;
	} else {
		iStatus = wl_wlif_get_wps_status_code();
		switch (iStatus) {
			case WLIF_WPS_UI_OK:
				status = "Success";
				break;

			case WLIF_WPS_UI_FINDING_PBC_STA:
			case WLIF_WPS_UI_FIND_PBC_AP: /* For STA mode */
				status = "In_progress";
				break;

			case WLIF_WPS_UI_PBCOVERLAP:
			case WLIF_WPS_UI_ERR:
				status = "Failed";
				break;

		default:
			status = "Idle";
			break;
		}
		snprintf(buf, bufLen, status);
		return 0;
	}
}

int
wldm_wfa_wps(wldm_wfa_wps_param_t *param)
{
	int ret = -1;

	if (!param) {
		WIFI_ERR("%s: null param\n", __FUNCTION__);
		return -1;
	}

	WIFI_DBG("%s: apIndex=%d cmd=%d\n", __FUNCTION__, param->apIndex, param->cmd);
	switch (param->cmd) {
		case WFA_WPS_ACTIVATE_PUSH_BUTTON:
			ret = wldm_WFA_WPS_ActivatePushButton(param->apIndex);
			break;
		case WFA_WPS_SET_CLIENT_PIN:
			ret = wldm_WFA_WPS_SetClientPin(param->apIndex, param->param.pin);
			break;
		case WFA_WPS_CANCEL:
			ret = wldm_WFA_WPS_Cancel(param->apIndex);
			break;
		case WFA_WPS_GET_STATUS:
			ret = wldm_WFA_WPS_GetStatus(param->apIndex, param->param.status, sizeof(param->param.status));
			break;
		default:
			WIFI_ERR("%s: cmd %d not supported\n", __FUNCTION__, param->cmd);
			return -1;
	}
	return ret;
}
/* End of WFA WPS */

/* BandSteering parameters support */

/* get/set 3 thresholds (util/rssi/phyrate) from bsd_steering_policy */
static int
wl_bsd_steering_policy(int cmd, int radioIndex, wldm_xbrcm_bsd_param_id_t param_id,
	int *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *nvifname, *osifname, nvName[STRING_LENGTH_32] = {0};
	char *nvValue = NULL, setValue[STRING_LENGTH_64] = {0};
	bsd_steering_policy_t policy;
	int count = 0, band = 1;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);

	if (cmd == CMD_LIST) {
		char *parameter;
		/* List the name only. */
		switch (param_id) {
			case WLDM_BSD_STEER_BANDUTIL:
				parameter = "BandSteeringBandUtilizationThreshold";
				break;

			case WLDM_BSD_STEER_RSSI:
				parameter = "BandSteeringRSSIThreshold";
				break;

			case WLDM_BSD_STEER_PHYRATE:
				parameter = "BandSteeringPhyRateThreshold";
				break;

			default:
				WIFI_ERR("%s: param_id[%d] not support\n",
					__FUNCTION__, param_id);
				return -1;
		}
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if ((nvifname = wldm_get_radio_nvifname(radioIndex)) == NULL) {
		WIFI_ERR("%s: null interface\n", __FUNCTION__);
		return -1;
	}

	osifname = wldm_get_radio_osifname(radioIndex);

	snprintf(nvName, sizeof(nvName), "%s_bsd_steering_policy", nvifname);
	nvValue = nvram_get(nvName);
	WIFI_DBG("%s: nvram_get(%s) value[%s] for nvifname=%s param_id=%d\n",
		__FUNCTION__, nvName, nvValue ? nvValue : "null", nvifname, param_id);

	if (nvValue != NULL) {
		count = sscanf(nvValue, "%d %d %d %d %d 0x%x",
			&(policy.bwUtil), &(policy.samplePeriod),
			&(policy.consecutiveSampleCount), &(policy.rssiThreshold),
			&(policy.phyRateThreshold), &(policy.extFlag));
	}

	if (cmd & CMD_GET_NVRAM) {
		if ((nvValue == NULL) || (count != 6)) {
			WIFI_ERR("%s: Error to get nvram (%s)\n", __FUNCTION__, nvName);
			return -1;
		}

		switch (param_id) {
			case WLDM_BSD_STEER_BANDUTIL:
				*pvalue = policy.bwUtil;
				break;

			case WLDM_BSD_STEER_RSSI:
				*pvalue = policy.rssiThreshold;
				break;

			case WLDM_BSD_STEER_PHYRATE:
				*pvalue = policy.phyRateThreshold;
				break;

			default:
				WIFI_ERR("%s: param_id[%d] not support\n",
					__FUNCTION__, param_id);
				return -1;
		}
		WIFI_DBG("%s: get threshold %d for param_id (%d)\n",
			__FUNCTION__, *pvalue, param_id);
		return 0;
	}

	if (cmd & CMD_SET_NVRAM) {
		if ((nvValue == NULL) || (count != 6)) {
			/* use default */
			policy.samplePeriod = BSD_DEFAULT_SAMPLE_PERIOD;
			policy.consecutiveSampleCount = BSD_DEFAULT_CONSECUTIVE_SAMPLE_COUNT;
			policy.rssiThreshold = BSD_DEFAULT_RSSI_THRESHOLD;
			policy.phyRateThreshold = BSD_DEFAULT_PHYRATE_THRESHOLD;

			if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
				WIFI_ERR("%s: IOVAR to get %s band info failed\n", __FUNCTION__, osifname);
				return -1;
			}
			band = dtoh32(band);

			if (band == WLC_BAND_5G) {
				policy.bwUtil = BSD_DEFAULT_BW_UTIL_5G;
				policy.extFlag =BSD_DEFAULT_EXTENSION_FLAG_5G;
			}
			else {
				policy.bwUtil = BSD_DEFAULT_BW_UTIL_2G;
				policy.extFlag =BSD_DEFAULT_EXTENSION_FLAG_2G;
			}
		}

		switch (param_id) {
			case WLDM_BSD_STEER_BANDUTIL:
				policy.bwUtil = *pvalue;
				break;

			case WLDM_BSD_STEER_RSSI:
				policy.rssiThreshold = *pvalue;
				break;

			case WLDM_BSD_STEER_PHYRATE:
				policy.phyRateThreshold = *pvalue;
				break;

			default:
				WIFI_ERR("%s: param_id[%d] not support\n",
					__FUNCTION__, param_id);
				return -1;
		}

		snprintf(setValue, sizeof(setValue), "%d %d %d %d %d 0x%x",
			policy.bwUtil, policy.samplePeriod, policy.consecutiveSampleCount,
			policy.rssiThreshold, policy.phyRateThreshold, policy.extFlag);
		if (nvram_set(nvName, setValue)) {
			WIFI_ERR("%s nvram_set(%s, %s) FAIL.", __FUNCTION__, nvName, setValue);
			return -1;
		}
		/* BCAWLAN-227462 - TCXB7-3804 - Temp solution to store BSD values to flash */
		nvram_commit();
	}

	return 0;
}

static int
wl_BandSteeringCapability(int cmd, int radioIndex,
	bool *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "BandSteeringCapability";

	IGNORE_CMD_WARNING(cmd, CMD_SET | CMD_ADD | CMD_DEL | CMD_SET_IOCTL |
		CMD_GET_NVRAM | CMD_SET_NVRAM);
	UNUSED_PARAMETER(radioIndex);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		*pvalue = 1; /* dummy value, always supported */
		if (plen) {
			*plen = sizeof(*pvalue);
		}
		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	return 0;
}

static int
wl_BandSteeringEnable(int cmd, int radioIndex,
	bool *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "BandSteeringEnable";
	char *nvValue = NULL, setValue[STRING_LENGTH_32] = {0};
	int val = 0;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);
	UNUSED_PARAMETER(radioIndex);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvValue = nvram_get("bsd_role");
	if (nvValue) {
		val = atoi(nvValue);
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		*pvalue = (val == 3) ? 1 : 0;
		if (plen) {
			*plen = sizeof(*pvalue);
		}
		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%d\n", parameter, *pvalue);
	}

	if (cmd & (CMD_SET | CMD_SET_NVRAM)) {
		val = (*pvalue == 1) ? 3 : 0;
		snprintf(setValue, sizeof(setValue), "%d", val);

		if (nvram_set("bsd_role", setValue)) {
			WIFI_ERR("%s nvram_set(bsd_role, %s) FAIL.", __FUNCTION__, setValue);
			return -1;
		}
		/* BCAWLAN-227462 - TCXB7-3804 - Temp solution to store BSD values to flash */
		nvram_commit();
		/* take action to stop/start bsd */
		if (system("killall -q -15 bsd 2>/dev/null") == -1) {
			WIFI_ERR("%s: Coulnd't kill all instances of bsd\n", __FUNCTION__);
		}
		WIFI_DBG("%s stop bsd\n", __FUNCTION__);

		if (*pvalue == 1) {
			WIFI_DBG("%s restart bsd\n", __FUNCTION__);
			if (system("bsd &") == -1) {
				WIFI_ERR("%s: Couldn't restart bsd\n", __FUNCTION__);
			}
		}
	}

	return 0;
}

static int
wl_BandSteeringApGroup(int cmd, int radioIndex,
	char *pvalue, int *plen, char *pbuf, int *pbufsz)
{
	char *parameter = "BandSteeringApGroup";
	char *nvValue = NULL, idx_str[STRING_LENGTH_32], aplist[STRING_LENGTH_32] = {0};
	char *t = NULL, *group1 = NULL, ifnames[STRING_LENGTH_32] = {0}, *nvifname;
	int apidx;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL | CMD_SET_IOCTL);
	UNUSED_PARAMETER(radioIndex);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & (CMD_GET | CMD_GET_NVRAM)) {
		if ((nvValue = nvram_get("bsd_ifnames")) == NULL) {
			WIFI_DBG("%s BandSteering AP Group not defined\n", __FUNCTION__);
			/* return aplist (null) if nvram not defined */
		} else {
			/* format like "wl0 wl1" to "0,1" => 1,2 */
			strncpy(ifnames, nvValue, sizeof(ifnames) - 1);
			memset(idx_str, '\0', sizeof(idx_str));
			t = strtok(ifnames, " ");
			while (t != NULL) {
				if ((apidx = wldm_get_apindex(t)) < 0) {
					WIFI_ERR("%s wrong apIndex for %s\n", __FUNCTION__, t);
					return -1;
				}

				sprintf(idx_str, "%d", apidx + 1);
				strcat(aplist, idx_str);
				if ((t = strtok(NULL, " ")) != NULL) {
					strcat(aplist, ",");
				}
			}
		}

		if (*plen < (strlen(aplist) + 1)) {
			WIFI_ERR("%s input buffer length too short (%d %d)\n",
				__FUNCTION__, *plen, strlen(aplist) + 1);
			return -1;
		}

		strcpy(pvalue, aplist);
		*plen = strlen(pvalue) + 1;

		if (cmd & CMD_LIST)
			PRINT_BUF(pbuf, *pbufsz, "%s=%s\n", parameter, pvalue);
	}

	if (cmd & (CMD_SET | CMD_SET_NVRAM)) {
		/* convert "0,1" to "wl0 wl1" */
		strncpy(aplist, pvalue, sizeof(aplist) - 1); /* not change the incoming argument */
		group1 = strtok(aplist, ";"); /* only support the first group currently */
		t = strtok(group1, ",");
		memset(ifnames, '\0', sizeof(ifnames));

		while (t != NULL) {
			apidx = atoi(t) - 1;
			if ((nvifname = wldm_get_nvifname(apidx)) == NULL) {
				WIFI_ERR("%s Fail to get nvifname for %d\n", __FUNCTION__, apidx);
				return -1;
			}
			strcat(ifnames, nvifname);
			t = strtok(NULL, ",");
			if (t) {
				strcat(ifnames, " ");
			}
		}

		if (nvram_set("bsd_ifnames", ifnames)) {
			WIFI_ERR("%s nvram_set to ApGroup %s) FAIL.", __FUNCTION__, ifnames);
			return -1;
		}
	}

	return 0;
}

int
wldm_xbrcm_bsd(int cmd, int radioIndex, wldm_xbrcm_bsd_param_id_t param_id,
	void *param, int *paramlen)
{
	if (!param) {
		WIFI_ERR("%s: parameter null pointer\n", __FUNCTION__);
		return -1;
	}

	WIFI_DBG("%s: cmd=0x%x, param_id=%d\n", __FUNCTION__, cmd, param_id);

	switch (param_id) {
		case WLDM_BSD_STEER_CAP:
			return wl_BandSteeringCapability(cmd, 0,
				(bool *)param, paramlen, NULL, NULL);

		case WLDM_BSD_STEER_ENABLE:
			return wl_BandSteeringEnable(cmd, 0,
				(bool *)param, paramlen, NULL, NULL);

		case WLDM_BSD_STEER_APGROUP:
			return wl_BandSteeringApGroup(cmd, 0,
				(char *)param, paramlen, NULL, NULL);

		case WLDM_BSD_STEER_BANDUTIL:
		case WLDM_BSD_STEER_RSSI:
		case WLDM_BSD_STEER_PHYRATE:
			return wl_bsd_steering_policy(cmd, radioIndex, param_id,
				(int *)param, NULL, NULL, NULL);

		default:
			WIFI_ERR("%s: param_id %d not support\n", __FUNCTION__, param_id);
			return -1;
	}
}

/* End of BandSteering */

/* SSD (SoftBlock) */
static int
dm_ssd_enable(int cmd, int apIndex, bool *pvalue, uint *plen, char *pvar)
{
	char *nvValue = NULL, setValue[STRING_LENGTH_32] = {0};
	int val = 0, new_val = 0;

	UNUSED_PARAMETER(apIndex);
	UNUSED_PARAMETER(pvar);

	nvValue = nvram_get("ssd_enable");
	if (nvValue) {
		val = atoi(nvValue);
	}

	if (cmd & CMD_GET_NVRAM) {
		*pvalue = (val == 1) ? 1 : 0;
		if (plen) {
			*plen = sizeof(*pvalue);
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		new_val = (*pvalue == 1) ? 1 : 0;
		if (val != new_val) {
			snprintf(setValue, sizeof(setValue), "%d", new_val);

			if (nvram_set("ssd_enable", setValue)) {
				WIFI_ERR("%s: nvram_set(ssd_enable, %s) FAIL.", __FUNCTION__, setValue);
				return -1;
			}

			/* take action to stop/start ssd */
			if (system("killall -9 ssd 2>/dev/null") == -1) {
				WIFI_ERR("%s: Couldn't kill all instances of ssd\n",
						__FUNCTION__);
			}
			else {
				WIFI_DBG("%s: stopped ssd\n", __FUNCTION__);
			}

			if (new_val == 1) {
				WIFI_DBG("%s: restart ssd\n", __FUNCTION__);
				if (system("ssd &") == -1) {
					WIFI_ERR("%s: Couldn't restart ssd in the background\n",
							__FUNCTION__);
				}
			}
		}
	}

	return 0;
}

static int
dm_ssd_list(int cmd, int apIndex, void *pvalue, uint *plen, char *pvar)
{
	char buf[32];
	int ret = -1;

	UNUSED_PARAMETER(apIndex);
	UNUSED_PARAMETER(pvalue);
	UNUSED_PARAMETER(plen);
	UNUSED_PARAMETER(pvar);

	/* list maclist on public interface added by ssd to a file */
	if (cmd & CMD_GET) {
		snprintf(buf, sizeof(buf), "ssd_cli -l > /dev/null");
		WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
		ret = system(buf);
		if (ret != 0)
			WIFI_ERR("%s: Fail to run %s\n", __FUNCTION__, buf);
	}

	/* clear maclist on public interface added by ssd */
	if (cmd & CMD_SET) {
		snprintf(buf, sizeof(buf), "ssd_cli -c");
		WIFI_DBG("%s: %s\n", __FUNCTION__, buf);
		ret = system(buf);
		if (ret != 0)
			WIFI_ERR("%s: Fail to run %s\n", __FUNCTION__, buf);
	}

	return ret;
}

static xbrcm_t xbrcm_ssd_tbl[] = {
	{  "softblock_enable",		{dm_ssd_enable},	CMD_GET_NVRAM | CMD_SET_NVRAM,	},
	{  "softblock_list",		{dm_ssd_list},		CMD_GET | CMD_SET,		},
	{  NULL,			{NULL},			0,				},
};

int
wldm_xbrcm_ssd(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);
	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_ssd_tbl);
}

/* End of SSD */

/* For factory restore */
/* Note: the input index can be either radio or ap index */
int
wldm_xbrcm_factory_reset(wldm_xbrcm_factory_reset_cmd_t cmd_id,
	int index, int commit, int restart)
{
	char cmd[BUF_SIZE], grep_key[STRING_LENGTH_32], *apsec_words;
	int radio_idx, bss_idx, max_radios, max_aps;
#ifdef PHASE2_SEPARATE_RC
	char *ifname;
#endif /* PHASE2_SEPARATE_RC */

	max_radios = wldm_get_radios();
	max_aps = wldm_get_max_aps();

	/* sync-up with all current settings first */
	for (radio_idx = 0; radio_idx < max_radios; radio_idx++) {
		/* XXX use "-1" to defer all actions, otherwise
		   cosa factory reset thread will suspend when taking some action */
		if (wldm_apply(radio_idx, -1) != 0) {
			WIFI_ERR("%s: wldm_apply fail for radio_idx=%d\n", __FUNCTION__, radio_idx);
		}
	}

	switch (cmd_id) {
		case WLDM_NVRAM_FACTORY_RESTORE:
			/* ask wifi_setup.sh to restore factory when init */
			if (system("erase") == -1) {
				WIFI_ERR("%s: Couldn't restore factory settings\n",
						__FUNCTION__);
			}
			WIFI_DBG("%s: cmd_id=%d erase old nvram etc.\n", __FUNCTION__, cmd_id);
			break;

		case WLDM_NVRAM_FACTORY_RESET_RADIO:
			if (index < 0 || index >= max_radios) {
				WIFI_ERR("%s: radio index %d exceeds range\n", __FUNCTION__, index);
				return -1;
			}
			snprintf(grep_key, sizeof(grep_key), "wl%d_", index);
			snprintf(cmd, sizeof(cmd), "grep %s %s > %s", grep_key,
				NVRAM_FACTORY_DEFAULT_RADIO, NVRAM_FACTORY_DEFAULT_TMP);
			break;

		case WLDM_NVRAM_FACTORY_RESET_AP:
			/* fall through */
		case WLDM_NVRAM_FACTORY_RESET_APSEC:
			if (index < 0 || index >= max_aps) {
				WIFI_ERR("%s: ap index %d exceeds range\n", __FUNCTION__, index);
				return -1;
			}
			radio_idx = wldm_get_radioIndex(index);
			bss_idx = wldm_get_bssidx(index);

			if (bss_idx == 0)
				snprintf(grep_key, sizeof(grep_key), "wl%d_", radio_idx);
			else
				snprintf(grep_key, sizeof(grep_key), "wl%d.%d_", radio_idx, bss_idx);

			if (cmd_id == WLDM_NVRAM_FACTORY_RESET_AP) {
				snprintf(cmd, sizeof(cmd), "grep %s %s > %s", grep_key,
					NVRAM_FACTORY_DEFAULT_AP, NVRAM_FACTORY_DEFAULT_TMP);
			}
			else {
				apsec_words = "akm=|wpa_psk=|crypto=|auth_mode=|auth=|wpa_gtk_rekey=|_radius_";
				snprintf(cmd, sizeof(cmd), "grep %s %s | egrep \"%s\" > %s", grep_key,
					NVRAM_FACTORY_DEFAULT_AP, apsec_words, NVRAM_FACTORY_DEFAULT_TMP);
			}
			break;

		default:
			WIFI_ERR("%s: reset cmd (%d) not supported\n", __FUNCTION__, cmd_id);
			return -1;
	}

	if (cmd_id != WLDM_NVRAM_FACTORY_RESTORE) {
		/* prepare nvram then load */
		if (system(cmd) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, cmd);
		}
		snprintf(cmd, sizeof(cmd), "nvram load -t %s", NVRAM_FACTORY_DEFAULT_TMP);
		if (system(cmd) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, cmd);
		}
	}

	/* post action after nvram load */
	if (commit) {
		nvram_commit();
	}

	if (restart) {
		if (cmd_id == WLDM_NVRAM_FACTORY_RESTORE) {
			if (system("reboot") == -1) {
				WIFI_ERR("%s: Couldn't reboot\n", __FUNCTION__);
			}
		}
		else {
			radio_idx = wldm_get_radioIndex(index);
#ifdef PHASE2_SEPARATE_RC
			/* for radio/ap, restart the individual interface */
			ifname = wldm_get_radio_osifname(radio_idx);
			snprintf(cmd, sizeof(cmd), "wifi_setup.sh restart %s", ifname);
			WIFI_DBG("%s: cmd=<%s>\n", __FUNCTION__, cmd);
			if (system(cmd) == -1) {
				WIFI_ERR("%s: Coulnd't issue %s\n",
						__FUNCTION__, cmd);
			}
#else /* PHASE2_SEPARATE_RC */
			wlcsm_mngr_restart(radio_idx, WLCSM_MNGR_RESTART_HTTPD, WLCSM_MNGR_RESTART_NOSAVEDM, 1);
#endif
		}
	}

	WIFI_DBG("%s: Done for cmd %d\n", __FUNCTION__, cmd_id);
	return 0;
}

/* For 802.11K support in wifi_hal.c */

/* Dummy start values to see easily in packet */
static unsigned char wifi_api_rm_DialogToken = 0x89;
static unsigned char wifi_api_mm_token = 0x99;

/* Used for Beacon Request, Report tokens such as wl1_wlcsm_mm_token
 * Using (static) vars locally does not work since it is not shared across threads
 */
static int
wl_get_token(char *nv_name)
{
	char  *valp = NULL;
	unsigned int ival;

	if ((nv_name != NULL) && (valp = wlcsm_nvram_get(nv_name)) != NULL) {
		ival = atoi(valp);
	}
	else {
		ival = 0x99; /* dummy value */
	}
	return (ival);
}

static int
wl_update_token(char *ifname, char *tokenname)
{
	char nv_name[64];
	unsigned char tval = 0;
	char aval[4];

	snprintf(nv_name, sizeof(nv_name), "%s_%s", ifname, tokenname);
	tval = wl_get_token(nv_name);
	if (++tval == 0)
		++tval;
	memset(aval, 0, sizeof(aval));
	snprintf(aval, sizeof(aval), "%3d", tval);
	wlcsm_nvram_set(nv_name, aval);
	return (tval);
}

/* Build and send Radio Measurement Request Action Frame
 * Input: *in_request is wifi_BeaconRequest_t for BeaconRequest
 * Output: *out_DialogToken is the measurement token
 */
static int
wl_send_rrm_req_actionFrame(char *osifname, uint8 *peer, uint32 channel, int32 dwell_time,
	uint8 rm_ieType, void *in_rrm_request, uint8 in_rrm_reqLen,
	unsigned char *out_DialogToken, unsigned short numRep)
{
	dot11_rmreq_t *rm_reqp;
	dot11_rm_ie_t *rm_iep;
	unsigned char *rm_req_measp;
	wl_af_params_t *afparamp;
	wl_action_frame_t *afInfo;
	char *iovbufp = NULL;
	uint iovblen, namelen;
	int afInfoLen, ret = -1;

	if (!(peer) || !(in_rrm_request) || !(out_DialogToken)) {
	    printf("%s WLCSM_GEN_ERR \n", __FUNCTION__);
	    return -1;
	}

	/* Allocate iov buffer */
	namelen = strlen("actframe") + 1;
	iovblen = sizeof(wl_af_params_t) + namelen;
	iovbufp = (char *)malloc(iovblen);
	if (iovbufp == NULL) {
		WIFI_ERR("%s: malloc failed!\n", __FUNCTION__);
		return -1;
	}
	memset(iovbufp, 0, iovblen);

	WIFI_DBG("%s iovbufp=%p iovblen=%d\n", __FUNCTION__, iovbufp, iovblen);

	memcpy(iovbufp, "actframe", namelen);
	afparamp = (wl_af_params_t *)(iovbufp + namelen);
	afparamp->channel = channel;
	afparamp->dwell_time = dwell_time;
	memset((&afparamp->BSSID), 0xFF, sizeof(afparamp->BSSID));

	/* Build action frame */

	afInfo = &afparamp->action_frame;
	afInfoLen = DOT11_RMREQ_LEN + DOT11_RM_IE_LEN + in_rrm_reqLen;
	bcopy(peer, (afInfo->da.octet), 6);
	afInfo->len = afInfoLen;
	afInfo->packetId = (unsigned int)(afInfo);

	WIFI_DBG("%s: Build action frame afInfoLen=%d in_rrm_reqLen=%d\n", __FUNCTION__,
		afInfoLen, in_rrm_reqLen);

	wifi_api_rm_DialogToken = wl_update_token(osifname, "rm_token");
	rm_reqp = (dot11_rmreq_t *)(afInfo->data);
	rm_reqp->category = DOT11_ACTION_CAT_RRM;
	rm_reqp->action = DOT11_RM_ACTION_RM_REQ;
	rm_reqp->token = wifi_api_rm_DialogToken;
#ifdef IL_BIGENDIAN
	rm_reqp->reps = hton16(numRep);
#else
	rm_reqp->reps = numRep;
#endif

	wifi_api_mm_token =  wl_update_token(osifname, "mm_token");
	rm_iep = (dot11_rm_ie_t *)(rm_reqp->data);
	rm_iep->id = DOT11_MNG_MEASURE_REQUEST_ID;
	rm_iep->len = in_rrm_reqLen + 3; /* add for meas token, mode, type */
	rm_iep->token = wifi_api_mm_token;
	rm_iep->mode = DOT11_RMREQ_MODE_REQUEST;
	rm_iep->type = rm_ieType;

	rm_req_measp = (unsigned char *)(rm_reqp) + afInfoLen - in_rrm_reqLen;
	memcpy((int8 *)(rm_req_measp), (int8 *)(in_rrm_request), in_rrm_reqLen);
	*out_DialogToken = rm_iep->token;

	WIFI_DBG("%s: call %s actframe reps=0x%x rrm_req type=%d len=%d token=%d\n",
		__FUNCTION__, osifname, rm_reqp->reps, rm_iep->type, rm_iep->len, rm_iep->token);

	ret = wl_ioctl(osifname, WLC_SET_VAR, iovbufp, iovblen);
	if (ret < 0) {
		WIFI_ERR("%s: %s fail to send actframe err=%d\n", __FUNCTION__, osifname, ret);
	} else {
		WIFI_DBG("%s: %s Sent actframe\n", __FUNCTION__, osifname);
	}
	if (iovbufp)
		free(iovbufp);
	return ret;
}

/* Generate Radio Measurement Request Action Frame requests
 * - currently only used by wifi_setRMBeaconRequest from wifi_hal
 * Input: af_reqp has parameter info from hal collected into wl_af_rrm_req_info_t
 * Output: outbuf has out_DialogToken
 */
static int
wl_11k_action_rm_req(int apIndex, unsigned char *peer_mac, wl_af_rrm_req_info_t *af_reqp,
	int *plen, unsigned char *outbuf, int *outbufsz)
{
	int ret = 0;
	char *osifname, eabuf[ETHER_ADDR_STR_LEN];

	if (!af_reqp || !(peer_mac) || !(af_reqp->in_request)) {
		WIFI_ERR("%s: apIndex=%d Null input data\n", __FUNCTION__, apIndex);
		return -1;
	}

	WIFI_DBG("%s: apIndex=%d af_reqp=%p *plen=%d peer=%s\n", __FUNCTION__, apIndex,
		af_reqp, *plen, ether_etoa(peer_mac, eabuf));

	osifname = wldm_get_osifname(apIndex);
	switch (af_reqp->rm_ieType) {
		case DOT11_MEASURE_TYPE_BEACON:
			/* Don't support optional elements yet */
			if (af_reqp->in_reqLen < RM_BCN_REQ_MANDATORY_ELEM_LEN) {
				WIFI_ERR("%s: in_reqLen %d too short\n", __FUNCTION__,
					af_reqp->in_reqLen);
				return -1;
			}
			WIFI_DBG("%s: wl_send_rrm_req_actionFrame type=0x%x len=%d numRep=%d\n",
				__FUNCTION__, af_reqp->rm_ieType, RM_BCN_REQ_MANDATORY_ELEM_LEN,
				af_reqp->numRepetitions);

			/* send action frame; no off channel request */
			ret = wl_send_rrm_req_actionFrame(osifname, (unsigned char *)peer_mac, 0,
				-1, af_reqp->rm_ieType, af_reqp->in_request,
				af_reqp->in_reqLen, outbuf, af_reqp->numRepetitions);
			if (ret < 0) {
				WIFI_ERR("%s: Error=%d send DOT11_MEASURE_TYPE_BEACON actframe\n",
					__FUNCTION__, ret);
				return -1;
			}
			WIFI_DBG("%s: Sent DOT11_MEASURE_TYPE_BEACON actframe out_DialogToken=%d\n",
				__FUNCTION__, (unsigned char)(*outbuf));
			break;

		default:
			break;
	}
	return ret;
}

/* rrm_cap by spec is 5 bytes, current nvram is only to lower 4 bytes of rrm_cap */
/* iovar supports 5 bytes though */
/* rrm ioctl requires wl down - need to use CMD_SET */
int
wldm_xbrcm_AccessPoint_RMCapabilities(int cmd, int apIndex, unsigned char *pvalue, int *plen,
	char *pbuf, int *pbufsz)
{
	char *parameter = "RMCapabilities";
	char *osifname, *nvifname;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	nvifname = wldm_get_nvifname(apIndex);
	osifname = wldm_get_osifname(apIndex);
	if (cmd & CMD_GET) {
		unsigned char rrm_val[DOT11_RRM_CAP_LEN] = {0};

		if (*plen < DOT11_RRM_CAP_LEN) {
			WIFI_ERR("%s: Buffer too short\n", __FUNCTION__);
			return -1;
		}
		if (wl_iovar_get(osifname, "rrm", rrm_val, DOT11_RRM_CAP_LEN) < 0) {
			WIFI_ERR("%s: wl_iovar_get() rrm failed! apIndex[%d]\n",
				__FUNCTION__, apIndex);
			return -1;
		}
		memcpy(pvalue, rrm_val, DOT11_RRM_CAP_LEN);
		WIFI_DBG("%s: Got rrm "RMCAPF" apIndex[%d]\n", __FUNCTION__,
			RMCAP_TO_RMCAPF(rrm_val), apIndex);
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_AccessPoint_Object *pObj = wldm_get_X_BROADCOM_COM_AccessPointObject(
			apIndex, X_BROADCOM_COM_AccessPoint_RMCapabilities_MASK);

		if (pObj == NULL)
			return -1;

		if (*plen != DOT11_RRM_CAP_LEN) {
			WIFI_ERR("%s: Error buffer length %d\n", __FUNCTION__, *plen);
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}
		WIFI_DBG("%s: SET "RMCAPF" len=%d apIndex[%d]\n", __FUNCTION__,
				RMCAP_TO_RMCAPF(pvalue), *plen, apIndex);
		memcpy(pObj->Ap.RMCapabilities, pvalue, DOT11_RRM_CAP_LEN);
		pObj->apply_map |= X_BROADCOM_COM_AccessPoint_RMCapabilities_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_GET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE], *nvram_value;
		unsigned int rrm_int;

		snprintf(nvram_name, sizeof(nvram_name), "%s_rrm", nvifname);
		nvram_value = wlcsm_nvram_get(nvram_name);
		rrm_int = (!nvram_value) ? 0 : (unsigned int)(strtoul(nvram_value, NULL, 16));
		if (*plen < sizeof(rrm_int)) {
			WIFI_ERR("%s: Buffer too short\n", __FUNCTION__);
			return -1;
		}
		memcpy(pvalue, (unsigned char *)(&rrm_int), sizeof(rrm_int));
		*plen = sizeof(rrm_int);
		WIFI_DBG("%s: CMD_GET_NVRAM plen=%d rrm_int=0x%08x apIndex[%d]\n", __FUNCTION__,
				*plen, rrm_int, apIndex);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE], nvrStr[STRING_LENGTH_32] = {0};
		unsigned int rrm_int;

		if (*plen < sizeof(rrm_int)) {
			WIFI_ERR("%s: Buffer too short\n", __FUNCTION__);
			return -1;
		}
		memcpy((unsigned char *)(&rrm_int), pvalue, sizeof(rrm_int));
		snprintf(nvram_name, sizeof(nvram_name), "%s_rrm", nvifname);
		snprintf(nvrStr, sizeof(nvrStr), "0x%x", rrm_int);
		NVRAM_SET(nvram_name, nvrStr);
		WIFI_DBG("%s: CMD_SET_NVRAM plen=%d rrm_int=0x%08x apIndex[%d]\n", __FUNCTION__,
				*plen, rrm_int, apIndex);
	}

	if (cmd & CMD_SET_IOCTL) {
		if (*plen != DOT11_RRM_CAP_LEN) {
			WIFI_ERR("%s: Error buffer length %d\n", __FUNCTION__, *plen);
			return -1;
		}
		if (wl_iovar_set(osifname, "rrm", pvalue, DOT11_RRM_CAP_LEN) != 0) {
			WIFI_ERR("%s: Error iovar_set rrm apIndex[%d]\n", __FUNCTION__, apIndex);
			return -1;
		}
		WIFI_DBG("%s: CMD_SET_IOCTL "RMCAPF" len=%d apIndex[%d]\n", __FUNCTION__,
			RMCAP_TO_RMCAPF(pvalue), *plen, apIndex);
	}
	return 0;
}

static boolean
wl_is_sta_in_assoclist(char *osifname, unsigned char *macaddr)
{
	struct ether_addr *ea = (struct ether_addr *) macaddr;
	char ioctl_buf[WLC_IOCTL_MEDLEN];
	struct maclist *assoclist;
	int assoc_cnt, cnt;

	if (!(wl_get_assoclist(osifname, ioctl_buf, sizeof(ioctl_buf)))) {
		assoclist = (struct maclist *) ioctl_buf;
		assoc_cnt = dtoh32(assoclist->count);
		if (assoc_cnt <= 0) {
			return FALSE;
		}
		for (cnt = 0; cnt < assoc_cnt; cnt++) {
			if (!(memcmp(&assoclist->ea[cnt], ea, sizeof(*ea)))) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

static int
wl_get_sta_rrm_cap(char *osifname, unsigned char *macaddr, unsigned char *rrmCap)
{
	char ioctl_buf[MAX_IOCTL_BUFLEN];
	sta_info_v8_t *sta_info_io;
	struct ether_addr *ea = (struct ether_addr *) macaddr;

	/* expect sta in assoclist; get rrmCap from sta_info */
	if (wl_get_sta_info(osifname, ea, ioctl_buf, sizeof(ioctl_buf)) < 0) {
		WIFI_ERR("%s osifname=%s sta_info error\n", __FUNCTION__, osifname);
		return -1;
	}
	sta_info_io = (sta_info_v8_t *)ioctl_buf;
	if (sta_info_io->ver < WL_STA_VER_8) {
		WIFI_ERR("%s osifname=%s sta_info ver is %d need %d\n", __FUNCTION__,
			osifname, sta_info_io->ver, WL_STA_VER_8);
		return -1;
	}
	memcpy(rrmCap, sta_info_io->rrm_capabilities, DOT11_RRM_CAP_LEN);
	WIFI_DBG("%s %s "MACF" rmcap "RMCAPF"\n", __FUNCTION__, osifname,
		ETHERP_TO_MACF(ea), RMCAP_TO_RMCAPF(rrmCap));
	return 0;
}

static int
wl_11k_getRMCapabilities(unsigned char *peer_mac, unsigned char *outcap)
{
	int apIndex, max_aps = wldm_get_max_aps();
	char *osifname;

	if ((peer_mac == NULL) || (outcap == NULL)) {
		WIFI_ERR("%s: Error input param\n", __FUNCTION__);
		return -1;
	}
	memset((unsigned char *) outcap, 0, DOT11_RRM_CAP_LEN);
	/* with iov do wl -i wlx.y sta_info <peer_macStr> |grep RRM |awk '{print $4}' */
	for (apIndex = 0; apIndex < max_aps; apIndex++) {
		if (!wldm_ap_enabled(apIndex)) {
			continue;
		}
		osifname = wldm_get_osifname(apIndex);
		if (wl_is_sta_in_assoclist(osifname, peer_mac) == TRUE) {
			return (wl_get_sta_rrm_cap(osifname, peer_mac, outcap));
		}
	}
	return -1;
}

static int
wl_11k_setNeighborReports(unsigned int apIndex, dot11_neighbor_rep_ie_t *neighborReports,
		unsigned int count)
{
	char *osifname;
	unsigned char mac_addr[ETHER_ADDR_LEN];
	int nbr_rep_len, i;
	dot11_neighbor_rep_ie_t *nrp = neighborReports;

	if (nrp == NULL) {
		WIFI_ERR("%s: Error Null nrp apIndex[%d]\n", __FUNCTION__, apIndex);
		return -1;
	}
	osifname = wldm_get_osifname(apIndex);
	WIFI_DBG("%s: Delete current neighbor list apIndex[%d]\n", __FUNCTION__, apIndex);

	/* broadcast mac to del all static neighbor reports */
	memset(mac_addr, 0xFF, sizeof(mac_addr));
	if (wl_iovar_set(osifname, "rrm_nbr_del_nbr", (char *)mac_addr, ETHER_ADDR_LEN) != 0) {
		WIFI_ERR("%s: Error del current list apIndex[%d]\n", __FUNCTION__, apIndex);
		return -1;
	}

	WIFI_DBG("%s: Add new neighbor list apIndex[%d]\n", __FUNCTION__, apIndex);

	/* Add each Static Neighbor Report */
	for (i = 0; i < count; i++) {
		nbr_rep_len = nrp->len + TLV_HDR_LEN;
		WIFI_DBG("%s: nrp=%p nbr_rep_len=%d id=%d len=%d\n", __FUNCTION__, nrp, nbr_rep_len,
			nrp->id, nrp->len);
		if (wl_iovar_set(osifname, "rrm_nbr_add_nbr", (char *)nrp, nbr_rep_len) < 0) {
			WIFI_ERR("%s: Error add_nbr apIndex[%d]\n", __FUNCTION__, apIndex);
			return -1;
		}
		/* to next report */
		nrp = (dot11_neighbor_rep_ie_t *)((unsigned char *)nrp + nbr_rep_len);
	}

	return 0;
}

/* peer_mac is 6 bytes long */
int
wldm_11k_rrm_cmd(wldm_rrm_cmd_t action, int apIndex, unsigned char *peer_mac,
	unsigned char *pvalue, int *plen, unsigned char *outBuf, int *outlen)
{
	wl_af_rrm_req_info_t *reqp;

	WIFI_DBG("%s: action=%d\n", __FUNCTION__, action);

	switch (action) {
		case WLDM_RRM_SEND_REQUEST:
			reqp = (wl_af_rrm_req_info_t *)(pvalue);
			if (reqp->rm_actionId == DOT11_RM_ACTION_RM_REQ) {
				return wl_11k_action_rm_req(apIndex, peer_mac, reqp,
					plen, outBuf, outlen);
			} else {
				WIFI_ERR("%s: rm_actionId %d not supported\n", __FUNCTION__,
					reqp->rm_actionId);
				return -1;
			}

		case WLDM_RRM_GET_CLIENT_RRM_CAP:
			if (*outlen != DOT11_RRM_CAP_LEN) {
				WIFI_ERR("%s: Error buffer length %d\n", __FUNCTION__, *outlen);
				return -1;
			}
			return wl_11k_getRMCapabilities(peer_mac, outBuf);

		case WLDM_RRM_SET_NEIGHBOR_REPORTS:
			if (wl_11k_setNeighborReports(apIndex, (dot11_neighbor_rep_ie_t *)pvalue,
				(unsigned int)(*plen)) < 0) {
				return -1;
			}
			return 0;

		default:
			WIFI_ERR("%s: action %d not support\n", __FUNCTION__, action);
			return -1;
	}
}

int
wldm_11ax_twt(int cmd, unsigned int apIndex,
	void *pvalue, int *plen, char *pbuf, int *pbufsz)
{
#ifndef WL_TWT_LIST_VER
	WIFI_ERR("%s: Not supported idx=%d\n", __FUNCTION__, apIndex);
	return -1;
#else
	int ret=0;
	char *parameter = "AXtwt";

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);
	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (cmd & CMD_GET) {
		ret = wl_getHE_TWTSessions(apIndex, (void *)pvalue, plen);
		if (ret < 0) {
			WIFI_ERR("%s: wl_getHE_TWTSessions Failed. idx=%d, ret=%d\n",
				__FUNCTION__, apIndex, ret);
			return -1;
		}
	}

	if (cmd & CMD_SET_IOCTL) {
		wldm_twt_cmd_info_t *tcp = (wldm_twt_cmd_info_t *)(pvalue);

		if (tcp->twtCmd == WL_TWT_CMD_TEARDOWN) {
			ret = wl_teardownHE_TWT(apIndex,
				(wldm_twt_sess_info_t *)(tcp->twtCmdInfo));
			if (ret < 0) {
				WIFI_ERR("%s: WL_TWT_CMD_TEARDOWN idx=%d, ret=%d\n",
					__FUNCTION__, apIndex, ret);
				return -1;
			}
		}
		else if (tcp->twtCmd == WL_TWT_CMD_SETUP) {
			ret = wl_setupHE_TWT(apIndex,
				(wldm_twt_setup_info_t *)(tcp->twtCmdInfo));
			if (ret < 0) {
				WIFI_ERR("%s: WL_TWT_CMD_SETUP idx=%d, ret=%d\n",
					__FUNCTION__, apIndex, ret);
				return -1;
			}
		}
	}
	return 0;
#endif /* WL_TWT_LIST_VER */
}

/* For ACS support */
typedef struct dm_acs_info {
	const char	*dm_name;		/* Name of the dm */
	const char	*param;			/* Nvram and command line parameter name */
} dm_acs_info_t;

static dm_acs_info_t dm_acs_map[] = {
	{"acs_ifnames",				"acs_ifnames"},
	{"acs_no_restrict_align",		"acs_no_restrict_align"},
	{"acsd2_started",			"acsd2_started"},
	{"acs_access_category_en",		"acs_access_category_en"},
	{"acs_ap_inttrf_numsecs",		"acs_ap_inttrf_numsecs"},
	{"acs_ap_inttrf_thresh",		"acs_ap_inttrf_thresh"},
	{"acs_ap_inttrf_total_numsecs",		"acs_ap_inttrf_total_numsecs"},
	{"acs_ap_inttrf_total_thresh",		"acs_ap_inttrf_total_thresh"},
	{"acs_bgdfs_ahead",			"acs_bgdfs_ahead"},
	{"acs_bgdfs_avoid_on_far_sta",		"acs_bgdfs_avoid_on_far_sta"},
	{"acs_bgdfs_enab",			"acs_bgdfs_enab"},
	{"acs_bgdfs_fallback_blocking_cac",	"acs_bgdfs_fallback_blocking_cac"},
	{"acs_bgdfs_idle_frames_thld",		"acs_bgdfs_idle_frames_thld"},
	{"acs_bgdfs_txblank_threshold",		"acs_bgdfs_txblank_threshold"},
	{"acs_boot_only",			"acs_boot_only"},
	{"AutoChannelDwellTime",		"acs_chan_dwell_time"},
	{"acs_chan_flop_period",		"acs_chan_flop_period"},
	{"acs_chanim_num_segments",		"acs_chanim_num_segments"},
	{"acs_ci_scan_timeout",			"acs_ci_scan_timeout"},
	{"DfsRefreshPeriod",			"acs_ci_scan_timer"},
	{"AutoChannelRefreshPeriod",		"acs_cs_scan_timer"},
	{"acs_dfs",				"acs_dfs"},
	{"acs_dfs_reentry",			"acs_dfs_reentry"},
	{"acs_dfsr_activity",			"acs_dfsr_activity"},
	{"acs_dfsr_deferred",			"acs_dfsr_deferred"},
	{"acs_dfsr_immediate",			"acs_dfsr_immediate"},
	{"acs_excl_chans",			"acs_excl_chans"},
	{"acs_fcs_mode",			"acs_fcs_mode"},
	{"acs_ignore_txfail",			"acs_ignore_txfail"},
	{"acs_pref_chans",			"acs_pref_chans"},
	{"acs_scan_entry_expire",		"acs_scan_entry_expire"},
	{"acs_segment_chanim",			"acs_segment_chanim"},
	{"acs_sta_inttrf_numsecs",		"acs_sta_inttrf_numsecs"},
	{"acs_sta_inttrf_thresh",		"acs_sta_inttrf_thresh"},
	{"acs_start_on_nondfs",			"acs_start_on_nondfs"},
	{"acs_traffic_thresh_en",		"acs_traffic_thresh_en"},
	{"acs_tx_idle_cnt",			"acs_tx_idle_cnt"},
	{"acsd_scs_dfs_scan",			"acsd_scs_dfs_scan"},
	{"AcsDfsMoveBack",			"acs_dfs_move_back"},
	{"channel_weights",			"acs_channel_weights"},
	{"exclude_dfs",				"exclude_dfs"},
	{"zdfs_supp",				"acs_zdfs_2g"},
	{"zdfs_state",				"acs_bgdfs"},
	{"zdfs_preclr",				"acs_bgdfs_preclear_etsi"},
	{NULL, NULL},
};

static const char *
get_acs_param(const char *pvar)
{
	dm_acs_info_t *info = &dm_acs_map[0];

	while (info->dm_name) {
		if (!strcmp(info->dm_name, pvar)) {
			return info->param;
		}
		info++;
	}

	return NULL;
}

static int
dm_acsv2(int cmd, int index, int *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	const char *param_name = get_acs_param(pvar);
	char *nvifname = wldm_get_radio_nvifname(index);
	char *osifname = wldm_get_radio_osifname(index);
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE];
	char cmdBuf[STRING_LENGTH_128] = {0};
	char outBuf[STRING_LENGTH_128] = {0};

	if (!param_name) {
		WIFI_ERR("%s: %s is not a supported acsd2 parameter\n", __FUNCTION__, pvar);
		return -1;
	}

	if (cmd & CMD_GET) {
		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s get %s", ACS_CLI, osifname, param_name);
		if (syscmd(cmdBuf, outBuf, sizeof(outBuf)) != 0) {
			WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
			return -1;
		}

		*pvalue = atoi(&outBuf[0]);
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", nvifname, param_name);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value) {
			*pvalue = atoi(nvram_value);
			*plen = sizeof(*pvalue);
		} else {
			WIFI_ERR("%s: unable to get value from nvram %s\n", __FUNCTION__, param_name);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		NVRAM_INT_SET(nvifname, param_name, *pvalue);
	}

	if (cmd & CMD_SET_IOCTL) {
		pid_t pid = get_pid_by_name(ACSD);
		if (pid <= 0) {
			WIFI_ERR("%s: acsd2 is not running\n", __FUNCTION__);
			return -1;
		}

		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s set %s %d", ACS_CLI, osifname, param_name, *pvalue);
		if (system(cmdBuf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, cmdBuf);
		}
	}

	return 0;
}

static int
_wldm_get_channel_weights(char *src_ch_wt_str, int *dst_ch_wt_values, uint *dst_num_ch_wts)
{
	char *ch_or_wt_str, *delim = ",";
	char ch_wt_buf[WLDM_MAX_CH_LIST_LEN * 2 + 2];
	int list_length, i, ch_or_wt;

	memset(ch_wt_buf, 0, sizeof(ch_wt_buf));
	list_length = strlen(src_ch_wt_str);
	list_length = MIN(list_length, WLDM_MAX_CH_LIST_LEN * 2);
	memcpy(ch_wt_buf, src_ch_wt_str, list_length);
	strncat(ch_wt_buf, ",", list_length);
	ch_or_wt_str = strtok(ch_wt_buf, delim);
	for (i = 0; i < WLDM_MAX_CH_LIST_LEN; i++) {
		if (ch_or_wt_str == NULL)
			break;
		ch_or_wt = atoi(ch_or_wt_str);
		dst_ch_wt_values[i] = ch_or_wt;
		ch_or_wt_str = strtok(NULL, delim);
	}
	/* len is number of items in list */
	*dst_num_ch_wts = i;
	return 0;
}

static int
_wldm_set_channel_weights_str(int *src_ch_wt_values, uint src_num_ch_wts,
	uint dst_ch_wt_str_size, char *dst_ch_wt_str)
{
	char ch_or_wt_str[8];
	int i, cwlen;

	memset(dst_ch_wt_str, 0, dst_ch_wt_str_size);
	memset(ch_or_wt_str, 0, sizeof(ch_or_wt_str));
	/* len is number of items in list */
	for (i = 0; i < src_num_ch_wts; i++) {
		snprintf(ch_or_wt_str, sizeof(ch_or_wt_str), "%d,", src_ch_wt_values[i]);
		strncat(dst_ch_wt_str, ch_or_wt_str, strlen(ch_or_wt_str));
	}
	cwlen = strlen(dst_ch_wt_str);
	dst_ch_wt_str[cwlen - 1] = '\0';
	return 0;
}

static int
dm_acsv2_chan_wt(int cmd, int index, int *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	const char *param_name = get_acs_param(pvar);
	char *nvifname = wldm_get_radio_nvifname(index);
	char *osifname = wldm_get_radio_osifname(index);
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE];
	char cmdBuf[STRING_LENGTH_128 + BUF_SIZE * 2] = {0};
	char outBuf[BUF_SIZE] = {0};

	if (!param_name) {
		WIFI_ERR("%s: %s is not a supported acsd2 parameter\n", __FUNCTION__, pvar);
		return -1;
	}

	if (cmd & CMD_GET_NVRAM) {

		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", nvifname, param_name);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value) {
			_wldm_get_channel_weights(nvram_value, pvalue, plen);
		} else {
			WIFI_DBG("%s: unable to get nvram %s value, no channel_weights\n",
				__FUNCTION__, nvram_name);
			pvalue[0] = 0;
			*plen = 0;
		}
	}

	if (cmd & CMD_GET) {
		char *c, empty_keyword[] = "No channel weights specified";

		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s get %s", ACS_CLI, osifname, param_name);
		if (syscmd(cmdBuf, outBuf, sizeof(outBuf)) != 0) {
			WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
			return -1;
		}
		if ((c = strchr(outBuf, '\n'))) {
			*c++ = '\0';
		} else {
			WIFI_DBG("%s: unable to get cmd %s value, no channel_weights\n",
				__FUNCTION__, cmdBuf);
			pvalue[0] = 0;
			*plen = 0;
		}
		if (strncmp(outBuf, empty_keyword, strlen(empty_keyword))) {
			_wldm_get_channel_weights(outBuf, pvalue, plen);
		} else {
			WIFI_DBG("%s: unable to get cmd %s value, no channel_weights\n",
				__FUNCTION__, cmdBuf);
			pvalue[0] = 0;
			*plen = 0;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		char ch_wt_buf[WLDM_MAX_CH_LIST_LEN * 2 + 2];
		_wldm_set_channel_weights_str(pvalue, *plen, sizeof(ch_wt_buf), ch_wt_buf);
		NVRAM_STRING_SET(nvifname, param_name, ch_wt_buf);
	}

	if (cmd & CMD_SET_IOCTL) {
		char ch_wt_buf[WLDM_MAX_CH_LIST_LEN * 2 + 2];
		pid_t pid = get_pid_by_name(ACSD);
		if (pid <= 0) {
			WIFI_ERR("%s: acsd2 is not running\n", __FUNCTION__);
			return -1;
		}

		_wldm_set_channel_weights_str(pvalue, *plen, sizeof(ch_wt_buf), ch_wt_buf);
		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s set %s %s", ACS_CLI, osifname, param_name, ch_wt_buf);
		if (system(cmdBuf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, cmdBuf);
		}
	}

	return 0;
}

static int
wl_get_wl_band(char *osifname, int *wl_band)
{
	chanspec_t chspec = 0;

	if (wl_ioctl(osifname, WLC_GET_BAND, wl_band, sizeof(*wl_band)) < 0) {
		WIFI_ERR("%s: IOCTL to get band info failed\n", __FUNCTION__);
		return -1;
	}
	*wl_band = dtoh32(*wl_band);
	if (*wl_band == WLC_BAND_AUTO) {
		/* convert to specific operating band */
		if (wl_iovar_getint(osifname, "chanspec", (int*)&chspec) < 0) {
			WIFI_ERR("%s: wl_iovar_getint chanspec failed\n", __FUNCTION__);
			return -1;
		}
		*wl_band = CHSPEC_IS2G(chspec) ? WLC_BAND_2G :
			(CHSPEC_IS5G(chspec) ? WLC_BAND_5G : WLC_BAND_6G);
	}
	return 0;
}

static int
wl_get_per_chan_info(char *osifname, uint channel, char *buf, int length)
{
	chanspec_t chspec = 0;
	chanspec_band_t chspec_band = WL_CHANSPEC_BAND_2G;
	int wl_band = 0;

	if (wl_get_wl_band(osifname, &wl_band) < 0) {
		WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
		return -1;
	}
	chspec_band = (wl_band == WLC_BAND_6G) ? WL_CHANSPEC_BAND_6G :
		(wl_band == WLC_BAND_5G) ? WL_CHANSPEC_BAND_5G :
		WL_CHANSPEC_BAND_2G;

	chspec = wf_create_20MHz_chspec(channel, chspec_band);
	if (chspec == INVCHANSPEC) {
		WIFI_ERR("%s: Invalid chanspec:0x%x\n", __FUNCTION__, chspec);
		return -1;
	}

	return wl_iovar_getbuf(osifname, "per_chan_info", &chspec, sizeof(chspec), buf,
		length);
}

static int
wl_get_per_chanspec_info(char *osifname, chanspec_t chspec, uint32 *chspec_info)
{
	uint sub_channel;
	char chan_info_buf[BUF_SIZE] = {0};
	wldm_channel_t chan;
	int i, num_chans = sizeof(chan);

	*chspec_info = 0;
	wl_parse_chanspec(chspec, &chan);
	for (i = 0; i < num_chans; i++) {
		sub_channel = ((uint8 *) &chan)[i];
		if (!sub_channel) {
			break;
		}
		if (wl_get_per_chan_info(osifname, sub_channel, chan_info_buf,
			sizeof(chan_info_buf)) < 0) {
			WIFI_ERR("%s: IOVAR to get per chan info failed, chspec:0x%x\n",
				__FUNCTION__, chspec);
			return -1;
		}
		*chspec_info |= dtoh32(*(uint32 *)chan_info_buf);
	}
	return 0;
}

static int
wl_get_dfs_status_all(char *osifname, wl_dfs_status_all_t *dfs_status_all, int len)
{
	int ret, count;

	ret = wl_iovar_getbuf(osifname, "dfs_status_all", NULL, 0, dfs_status_all, len);
	if (ret < 0) {
		WIFI_ERR("%s: IOVAR to get dfs status all failed\n", __FUNCTION__);
		return -1;
	}

	dfs_status_all->version = dtoh16(dfs_status_all->version);
	if (dfs_status_all->version != WL_DFS_STATUS_ALL_VERSION) {
		WIFI_WARNING("%s: unsupported dfs status version %d\n",
			__FUNCTION__, dfs_status_all->version);
		ret = BCME_UNSUPPORTED;
		return ret;
	}
	dfs_status_all->num_sub_status = dtoh16(dfs_status_all->num_sub_status);
	for (count = 0; count < dfs_status_all->num_sub_status; ++count) {
		wl_dfs_sub_status_t *sub = &dfs_status_all->dfs_sub_status[count];

		if (sub->state >= WL_DFS_CACSTATES) {
			WIFI_WARNING("%s: unknown dfs state %d.\n", __FUNCTION__, sub->state);
			ret = -1;
			break;
		}
	}
	return ret;
}

static int
dm_acsv2_excl_dfs(int cmd, int index, boolean *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	const char *param_name = get_acs_param(pvar);
	char *nvifname = wldm_get_radio_nvifname(index);
	char *osifname = wldm_get_radio_osifname(index);
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE];
	int wl_band = 0;
	char cmdBuf[WLDM_MAX_CH_LIST_LEN * STRING_LENGTH_32 + STRING_LENGTH_64] = {0};
	int i, chanspecs_count = 0;
	chanspec_t chanspecs[WLDM_MAX_CH_LIST_LEN] = {0};
	uint8 control_channels[WLDM_MAX_CH_LIST_LEN] = {0};
	uint excl_chanspecs_len = 0;
	uint32 chspec_info;
	char chanspec_str[STRING_LENGTH_32];
	char excl_chanspecs[WLDM_MAX_CH_LIST_LEN * STRING_LENGTH_32 + 1];

	if (!param_name) {
		WIFI_ERR("%s: %s is not a supported acsd2 parameter\n", __FUNCTION__, pvar);
		return -1;
	}

	if (wl_get_wl_band(osifname, &wl_band) < 0) {
		WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
		return -1;
	}
	if (wl_band != WLC_BAND_5G) {
		WIFI_ERR("%s: %d is not supported band\n", __FUNCTION__, wl_band);
		return -1;
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s", nvifname, param_name);
		nvram_value = nvram_safe_get(nvram_name);
		*pvalue = (strtoul(nvram_value, NULL, 10)) ? TRUE : FALSE;
		*plen = sizeof(*pvalue);
	}

	if (cmd & (CMD_SET_NVRAM | CMD_SET_IOCTL)) {
		memset(excl_chanspecs, 0, sizeof(excl_chanspecs));
		memset(chanspec_str, 0, sizeof(chanspec_str));
		if (*pvalue) {
			if (wl_get_chanspecs_list(index, sizeof(chanspecs),
				chanspecs, control_channels, &chanspecs_count)) {
				WIFI_ERR("%s: Failed to get chanspecs\n", __FUNCTION__);
				return -1;
			}
			for (i = 0; i < chanspecs_count; i++) {
				if (chanspecs[i] == INVCHANSPEC) {
					WIFI_ERR("%s: Invalid chanspec:0x%x\n", __FUNCTION__,
						chanspecs[i]);
					continue;
				}
				if (wl_get_per_chanspec_info(osifname, chanspecs[i], &chspec_info)) {
					WIFI_ERR("%s: Failed to get per chanspec info, chspec:0x%x\n",
						__FUNCTION__, chanspecs[i]);
					return -1;
				}
				if (chspec_info & WL_CHAN_RADAR) {
					snprintf(chanspec_str, sizeof(chanspec_str), "0x%x,",
						chanspecs[i]);
					strncat(excl_chanspecs, chanspec_str, strlen(chanspec_str));
				}
			}

			excl_chanspecs_len = strlen(excl_chanspecs);
			if (excl_chanspecs_len > 0) {
				excl_chanspecs[excl_chanspecs_len - 1] = '\0';
			} else {
				WIFI_DBG("%s: No DFS chanspecs found\n", __FUNCTION__);
			}
		} else {
			snprintf(excl_chanspecs, sizeof(excl_chanspecs), "%d", 0);
		}
		if (cmd & CMD_SET_IOCTL) {
			pid_t pid = get_pid_by_name(ACSD);
			if (pid <= 0) {
				WIFI_ERR("%s: acsd2 is not running\n", __FUNCTION__);
				return -1;
			}
			snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s set %s %s",
				ACS_CLI, osifname, "acs_excl_chans", excl_chanspecs);
			if (system(cmdBuf) == -1) {
				WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, cmdBuf);
			}
		}
		if (cmd & CMD_SET_NVRAM) {
			NVRAM_STRING_SET(nvifname, "acs_excl_chans", excl_chanspecs);
			NVRAM_BOOL_SET(nvifname, param_name, *pvalue);
		}
	}

	return 0;
}

static int
dm_acsv2_zdfs_supp(int cmd, int index, boolean *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	const char *param_name = get_acs_param(pvar);
	char *osifname = wldm_get_radio_osifname(index), *tmp_osifname;
	char caps[WLC_IOCTL_MEDLEN] = {0};
	int i, wl_band = 0, tmp_wl_band = 0, zdfs_2g = 0;
	bool acs_zdfs_2g = FALSE, zdfs_2g_cap = FALSE, zdfs_5g_cap = FALSE;

	/* ZeroWait DFS only involves 2G and 5G, not 6G on index 2 */
	if (index >= 2) {
		WIFI_ERR("%s: invalid radio index %d for ZWDFS can only support on 2G and 5G.\n",
			__FUNCTION__, index);
	}

	if (!param_name) {
		WIFI_ERR("%s: %s is not a supported acsd2 parameter\n", __FUNCTION__, pvar);
		return -1;
	}
	acs_zdfs_2g = nvram_match(param_name, "1");

	if (wl_get_wl_band(osifname, &wl_band) < 0) {
		WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
		return -1;
	}

	if (cmd & CMD_GET) {
		*plen = sizeof(*pvalue);
		if (wl_band != WLC_BAND_5G) {
			WIFI_DBG("%s: %d is not supported band\n", __FUNCTION__, wl_band);
			*pvalue = FALSE;
			return 0;
		}

		for (i = 0; i < MAX_WLAN_ADAPTER; i++) {
			if ((tmp_osifname = wldm_get_radio_osifname(i)) == NULL) {
				WIFI_DBG("%s: radio index %d out of range!\n",
					__FUNCTION__, i);
				break;
			}
			if (wl_get_wl_band(tmp_osifname, &tmp_wl_band) < 0) {
				WIFI_ERR("%s: get wl band  failed\n", __FUNCTION__);
				return -1;
			}
			if (wl_iovar_get(tmp_osifname, "cap", (void *)caps, sizeof(caps)) < 0) {
				WIFI_ERR("%s: wl_iovar_get cap failed\n", __FUNCTION__);
				return -1;
			}
			if (acs_zdfs_2g && tmp_wl_band == WLC_BAND_2G) {
				if (wl_iovar_getint(tmp_osifname, "zdfs_2g", &zdfs_2g) < 0) {
					WIFI_ERR("%s: wl_iovar_getint zdfs_2g failed\n",
						__FUNCTION__);
					return -1;
				}
				zdfs_2g_cap = ((strstr(caps, "bgdfs160") != NULL) &&
					(zdfs_2g == 1));
			} else if (tmp_wl_band == WLC_BAND_5G) {
				zdfs_5g_cap = strstr(caps, "bgdfs") != NULL;
			}

			if (zdfs_2g_cap || zdfs_5g_cap)
				break;
		}

		*pvalue = (zdfs_2g_cap || zdfs_5g_cap) ? TRUE : FALSE;
	}

	return 0;
}

static int
dm_acsv2_zdfs_state(int cmd, int index, int *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	const char *param_name = get_acs_param(pvar);
	char *nvifname = wldm_get_radio_nvifname(index);
	char *osifname = wldm_get_radio_osifname(index);
	char *nvram_value, nvram_name[NVRAM_NAME_SIZE], nvram_set_value[STRING_LENGTH_32];
	char cmdBuf[STRING_LENGTH_128] = {0};
	char outBuf[STRING_LENGTH_128] = {0};

	if (!param_name) {
		WIFI_ERR("%s: %s is not a supported acsd2 parameter\n", __FUNCTION__, pvar);
		return -1;
	}

	if (cmd & CMD_GET) {
		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s get %s", ACS_CLI, osifname, param_name);
		if (syscmd(cmdBuf, outBuf, sizeof(outBuf)) != 0) {
			WIFI_ERR("%s: syscmd failed for %s!\n", __FUNCTION__, cmdBuf);
			return -1;
		}

		*pvalue = atoi(&outBuf[0]);
		*plen = sizeof(*pvalue);
	}

	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s_enab", nvifname, param_name);
		nvram_value = wlcsm_nvram_get(nvram_name);
		if (nvram_value) {
			*pvalue = atoi(nvram_value);
			*plen = sizeof(*pvalue);
		} else {
			WIFI_ERR("%s: unable to get value from nvram %s\n", __FUNCTION__, param_name);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		/* Set nvram wlx_acs_bgdfs_enab */
		snprintf(nvram_name, sizeof(nvram_name), "%s_%s_enab", nvifname, param_name);
		snprintf(nvram_set_value, sizeof(nvram_set_value), "%d", (*pvalue) ? 1 : 0);
		NVRAM_SET(nvram_name, nvram_set_value);

		/* Set nvram wlx_acs_allow_immediate_dfsr to opposite of *pvalue */
		snprintf(nvram_name, sizeof(nvram_name), "%s_acs_allow_immediate_dfsr", nvifname);
		snprintf(nvram_set_value, sizeof(nvram_set_value), "%d", (*pvalue) ? 0 : 1);
		NVRAM_SET(nvram_name, nvram_set_value);
	}

	if (cmd & CMD_SET_IOCTL) {
		pid_t pid = get_pid_by_name(ACSD);
		if (pid <= 0) {
			WIFI_ERR("%s: acsd2 is not running\n", __FUNCTION__);
			return -1;
		}

		snprintf(cmdBuf, sizeof(cmdBuf), "%s -i %s set %s %d", ACS_CLI, osifname, param_name, *pvalue);
		if (system(cmdBuf) == -1) {
			WIFI_ERR("%s: Couldn't issue %s\n", __FUNCTION__, cmdBuf);
		}
	}

	return 0;
}

static xbrcm_t xbrcm_acs_tbl[] = {
	{  "DfsRefreshPeriod",		{dm_acsv2},	CMD_GET | CMD_GET_NVRAM | CMD_SET_NVRAM | CMD_SET_IOCTL,	},
	{  "AutoChannelDwellTime",	{dm_acsv2},	CMD_GET | CMD_GET_NVRAM | CMD_SET_NVRAM | CMD_SET_IOCTL,	},
	{  "AcsDfsMoveBack",		{dm_acsv2},	CMD_GET | CMD_GET_NVRAM | CMD_SET_NVRAM | CMD_SET_IOCTL,	},
	{  "channel_weights",		{dm_acsv2_chan_wt},	CMD_GET | CMD_GET_NVRAM | CMD_SET_NVRAM | CMD_SET_IOCTL,	},
	{  "exclude_dfs",		{dm_acsv2_excl_dfs},	CMD_GET_NVRAM | CMD_SET_NVRAM | CMD_SET_IOCTL,		},
	{  "zdfs_supp",			{dm_acsv2_zdfs_supp},	CMD_GET,						},
	{  "zdfs_state",		{dm_acsv2_zdfs_state},	CMD_GET | CMD_SET_NVRAM | CMD_SET_IOCTL,		},
	{  "zdfs_preclr",		{dm_acsv2},	CMD_GET | CMD_SET_NVRAM | CMD_SET_IOCTL,			},
	{  NULL,			{NULL},		0,								},
};

int
wldm_xbrcm_acs(int cmd, int index, void *pvalue, int *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);
	return dm_xbrcm(cmd, index, pvalue, (uint *)plen, pvar, xbrcm_acs_tbl);
}

static xbrcm_t xbrcm_phy_tbl[] = {
	{  "txpwr",		{dm_txpwr},		CMD_GET,	},
	{  NULL,		{NULL},			0,		},
};

int
wldm_xbrcm_phy(int cmd, int radioIndex, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);
	return dm_xbrcm(cmd, radioIndex, pvalue, plen, pvar, xbrcm_phy_tbl);
}

/* Start of Passpoint */
#define NVRAM_HSFLAG_GET(ifname, nvbit, nvram_bool) \
	do { \
		char prefix[NVRAM_NAME_SIZE]; \
		snprintf(prefix, sizeof(prefix), "%s_", ifname); \
		*nvram_bool = get_hspot_flag(prefix, nvbit); \
	} while (0)

#define NVRAM_HSFLAG_SET(ifname, nvbit, nvram_bool) \
	do { \
		char prefix[NVRAM_NAME_SIZE]; \
		snprintf(prefix, sizeof(prefix), "%s_", ifname); \
		set_hspot_flag(prefix, nvbit, *(boolean *)nvram_bool); \
	} while (0)

/* Number of hspot related nvram set count. */
static int hspot_nvram_set_count = 0;

typedef enum {
	T_INT = 0,		/* integer value */
	T_STR,			/* string value */
	T_BIT,			/* bit value */
	T_BOOL			/* boolean value */
} VALUE_TYPE;

typedef struct dm_nvram_iovar_info {
	char	*dm_name;	/* name of the dm */
	char	*param;		/* nvram/iovar name */
	uint	hsflag_bit;	/* hot spot flag bit */
	boolean	if_flag;	/* per interface (TRUE) or global (FALSE) */
	int	value_type;	/* parameter type */
} dm_nvram_iovar_info_t;

static const dm_nvram_iovar_info_t dm_11u_iw_map[] = {
	{  "iw_enable",		NULL,		HSFLG_U11_EN,	TRUE,	T_BIT,	},
	{  "iw_internet_av",	NULL,		HSFLG_IWINT_EN,	TRUE,	T_BIT,	},
	{  "iw_net_type",	NVNM_IWNETTYPE,	0,		TRUE,	T_INT,	},
	{  "iw_asra",		NULL,		HSFLG_IWASRA_EN,TRUE,	T_BIT,	},
	{  "iw_esr",		NULL,		HSFLG_IWESR_EN,	TRUE,	T_BIT,	},
	{  "iw_uesa",		NULL,		HSFLG_IWUESA_EN,TRUE,	T_BIT,	},
	{  "iw_venue_grp",	NVNM_VENUEGRP,	0,		TRUE,	T_INT,	},
	{  "iw_venue_type",	NVNM_VENUETYPE,	0,		TRUE,	T_INT,	},
	{  "iw_hessid",		NVNM_HESSID,	0,		TRUE,	T_STR,	},
	{  "iw_capability",	"hspot",	0,		TRUE,	T_BOOL,	},
	{  NULL,		NULL,		0,		FALSE,	0,	},
};

static const dm_nvram_iovar_info_t dm_hspot_map[] = {
	{  "hs_hspot_ie",	NULL,		HSFLG_HS_EN,	TRUE,	T_BIT,	},
	{  "hs_cntry_ie",	"reg_mode",	0,		TRUE,	T_STR,	},
	{  "hs_p2p_cross",	NULL,		HSFLG_P2P_CRS,	TRUE,	T_BIT,	},
	{  "hs_dgaf_ds",	NULL,		HSFLG_DGAF_DS,	TRUE,	T_BIT,	},
	{  "hs_bss_load",	"bssload",	0,		TRUE,	T_BOOL,	},
	{  "hs_proxy_arp",	NULL,		HSFLG_PROXY_ARP,TRUE,	T_BIT,	},
	{  "hs_l2_trf",		NULL,		HSFLG_L2_TRF,	TRUE,	T_BIT,	},
	{  "hs_gas_qrlimit",	"advp_qrlimit",	0,		FALSE,	T_INT,	},
	{  "hs_oui_extra",	"oui_extra",	0,		TRUE,	T_INT,	},
	{  "hs_oui_list",	"ouilist",	0,		TRUE,	T_STR,	},
	{  NULL,		NULL,		0,		FALSE,	0,	},
};

static const dm_nvram_iovar_info_t *
get_dm_nvram_iovar_entry(const char *pvar, const dm_nvram_iovar_info_t *dm_map_table)
{
	const dm_nvram_iovar_info_t *info = dm_map_table;

	while (info->dm_name) {
		if (!strcmp(info->dm_name, pvar)) {
			return info;
		}
		info++;
	}

	return NULL;
}

static int
wldm_excute_nvram_map(int cmd, int apIndex, void *pvalue, uint *plen, char *pvar,
		int *pvarsz, const dm_nvram_iovar_info_t *dm_map_table)
{
	const dm_nvram_iovar_info_t *info = get_dm_nvram_iovar_entry(pvar, dm_map_table);
	char *osifname = wldm_get_osifname(apIndex);
	char *nvifname;

	if (!info) {
		WIFI_ERR("%s: %s is not a supported parameter\n", __FUNCTION__, pvar);
		return -1;
	}

	nvifname = info->if_flag ? wldm_get_nvifname(apIndex) : NULL;

	if (cmd & CMD_GET_NVRAM) {
		switch(info->value_type) {
		case T_INT:
			NVRAM_INT_GET(nvifname, info->param, pvalue);
			break;
		case T_STR:
			NVRAM_STRING_GET(nvifname, info->param, pvalue, plen);
			break;
		case T_BOOL:
			NVRAM_BOOL_GET(nvifname, info->param, pvalue);
			break;
		case T_BIT:
			NVRAM_HSFLAG_GET(nvifname, info->hsflag_bit, (boolean *)pvalue);
			break;
		default:
			WIFI_ERR("%s: type %d not supported\n", __FUNCTION__, info->value_type);
			return -1;
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		int	hspot_enabled;

		switch(info->value_type) {
		case T_INT:
			NVRAM_INT_SET(nvifname, info->param, *(int *)pvalue);
			break;
		case T_STR:
			NVRAM_STRING_SET(nvifname, info->param, (char *)pvalue);
			break;
		case T_BOOL:
			NVRAM_BOOL_SET(nvifname, info->param, *(boolean *)pvalue);
			break;
		case T_BIT:
			NVRAM_HSFLAG_SET(nvifname, info->hsflag_bit, pvalue);
			break;
		default:
			WIFI_ERR("%s: type %d not supported\n", __FUNCTION__, info->value_type);
			return -1;
		}

		/* Increase count (start hspotap) if hspot in nvram is enabled */
		NVRAM_INT_GET(nvifname, "hspot", &hspot_enabled);
		if (hspot_enabled) {
			hspot_nvram_set_count++;
		}
	}

	if (cmd & CMD_GET) {
		int int_value;

		switch(info->value_type) {
		case T_BOOL:
			IOVAR_INT_GET(osifname, info->param, &int_value);
			*(boolean *)pvalue = (int_value != 0) ? TRUE : FALSE;
			break;
		default:
			WIFI_ERR("%s: type %d not supported\n", __FUNCTION__, info->value_type);
			return -1;
		}
	}

	if (cmd & CMD_SET) {
		switch(info->value_type) {
		case T_BOOL:
			IOVAR_INT_SET(osifname, info->param, *(boolean *)pvalue ? 1 : 0);
			break;
		default:
			WIFI_ERR("%s: type %d not supported\n", __FUNCTION__, info->value_type);
			return -1;
		}
	}

	return 0;
}

int
wldm_11u_iw(int cmd, int apIndex, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	WIFI_DBG("%s: cmd=%d, apIndex=%d, dm_name=%s\n", __FUNCTION__, cmd, apIndex, pvar);
	return wldm_excute_nvram_map(cmd, apIndex, pvalue, plen, pvar, pvarsz, dm_11u_iw_map);
}

int
wldm_hspot(int cmd, int apIndex, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	WIFI_DBG("%s: cmd=%d, apIndex=%d, dm_name=%s\n", __FUNCTION__, cmd, apIndex, pvar);
	return wldm_excute_nvram_map(cmd, apIndex, pvalue, plen, pvar, pvarsz, dm_hspot_map);
}

void
wldm_hspot_restart_if_needed()
{
	pid_t pid;

	if (hspot_nvram_set_count) {
		hspot_nvram_set_count = 0;

		/* First to kill all hspotap process if already start*/
		pid = get_pid_by_name("hspotap");
		if (pid > 0) {
			if (system("killall -q -15 hspotap 2>/dev/null") == -1) {
				WIFI_ERR("%s Couldn't kill all instances of hspotap\n",
					__FUNCTION__);
			}
		}

		if (system("hspotap&") == -1) {
			WIFI_ERR("%s: Couldn't restart hotspot daemon in the background\n",
					__FUNCTION__);
		}
		else {
			WIFI_DBG("%s: hspotap restarted!\n", __FUNCTION__);
		}
	}

	return;
}

static he_mu_type_t
hefeatures2mutype(he_features_t he_features, vht_mu_features_t mu_features, link_direction_t direction)
{
	he_mu_type_t mu_type;
	WIFI_DBG("%s: he features = 0x%x mu_features = %d direction = %s\n", __FUNCTION__,
		 he_features, mu_features, (direction == HE_MU_DOWNLINK) ? "downlink" : "uplink");

	if (direction == HE_MU_DOWNLINK) {
		if ((he_features & WL_HE_FEATURES_DLOMU) &&
			(he_features & WL_HE_FEATURES_DLMMU)) {
			mu_type = HE_MU_DL_OFDMA_HEMUMIMO;
		} else if (he_features & WL_HE_FEATURES_DLOMU) {
			mu_type = HE_MU_DL_OFDMA;
		} else if (he_features & WL_HE_FEATURES_DLMMU) {
			mu_type = HE_MU_DL_HEMUMIMO;
		} else {
			mu_type = HE_MU_DL_NONE;
		}
	} else {
		if (he_features & WL_HE_FEATURES_ULOMU) {
			mu_type = HE_MU_UL_OFDMA;
		} else {
			mu_type = HE_MU_UL_NONE;
		}
	}

	WIFI_DBG("%s: mu type = %d he feature 0x%x\n", __FUNCTION__, mu_type, he_features);
	return mu_type;
}

static int
hemutype2features(he_mu_type_t mu_type, he_features_t *features)
{
	int ret = 0;

	WIFI_DBG("%s: input features = 0x%x\n", __FUNCTION__, *features);

	*features &= HE_FEATURES_DEFAULT;

	/* set he feature bit 2 to bit 5 */
	if (mu_type == HE_MU_DL_OFDMA) {
		*features &= ~(WL_HE_FEATURES_DLMMU);
		*features |= WL_HE_FEATURES_DLOMU;
	} else if (mu_type == HE_MU_UL_OFDMA) {
		*features |= (WL_HE_FEATURES_ULOMU | WL_HE_FEATURES_DLOMU | WL_HE_FEATURES_DLMMU);
	} else if (mu_type == HE_MU_DL_HEMUMIMO) {
		/* UL OFDMA doesn't work with DL HEMIMO only. This is not a tested mode */
		*features &= ~(WL_HE_FEATURES_DLOMU | WL_HE_FEATURES_ULOMU);
		*features |= WL_HE_FEATURES_DLMMU;
	} else if (mu_type == HE_MU_DL_OFDMA_HEMUMIMO) {
		*features |= (WL_HE_FEATURES_DLOMU | WL_HE_FEATURES_DLMMU);
	} else if (mu_type == HE_MU_DL_NONE) {
		/* Without DL OFDMA, UL OFDMA doesn't work */
		*features &= ~(WL_HE_FEATURES_DLOMU |
			WL_HE_FEATURES_DLMMU | WL_HE_FEATURES_ULOMU);
	} else if (mu_type == HE_MU_UL_NONE) {
		*features &= ~(WL_HE_FEATURES_ULOMU);
	} else {
		WIFI_ERR("%s: mu type %d is invalid\n", __FUNCTION__, mu_type);
		ret = -2;
	}

	if (ret == 0){
		/* set he feature bit 0 - bit 1 for 2.4G and 5G enable/disable */
		*features |= (WL_HE_FEATURES_2G | WL_HE_FEATURES_5G);
	}

	WIFI_DBG("%s: features (0x%x) generated from mu type %d\n", __FUNCTION__,
		*features, mu_type);
	return ret;
}

static int
is11AXCapable(char *osifname)
{
	char caps[WLC_IOCTL_MEDLEN];

	if (wl_iovar_get(osifname, "cap", (void *)caps, sizeof(caps))) {
		WIFI_ERR("%s: failed to get wifi capabilities\n", __FUNCTION__);
		return -1;
	}

	if (strstr(caps, "11ax") == NULL) {
		WIFI_ERR("%s: not supported without 11ax capability\n", __FUNCTION__);
		return -2;
	}

	WIFI_DBG("%s: 11ax supported\n", __FUNCTION__);
	return 0;
}

int
wldm_xbrcm_Radio_AXmuType(int cmd, int radioIndex, he_mu_type_t *mutype, uint *plen, char *pbuf, int *pbufsz)
{
	char *nvifname = wldm_get_radio_nvifname(radioIndex);
	char *osifname = wldm_get_radio_osifname(radioIndex);
	char nvram_name[NVRAM_NAME_SIZE];
	char *parameter = "HeMuType";
	he_features_t features;
	vht_mu_features_t mu_features = 0;
	link_direction_t direction;
	int ret;
	uint32 enable = 0;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (is11AXCapable(osifname) < 0) {
		WIFI_ERR("%s: failed to get wifi capabilities\n", __FUNCTION__);
		return -1;
	}

	if (wl_iovar_getint(osifname, "mu_features", (int *)&mu_features) < 0) {
		WIFI_ERR("%s: failed to get wifi mu_features\n", __FUNCTION__);
		return -2;
	}

	direction = (*mutype >= HE_MU_UL_NONE) ? HE_MU_UPLINK : HE_MU_DOWNLINK;

	if (cmd & (CMD_GET | CMD_SET_IOCTL | CMD_SET_NVRAM)) {
		ret = wl_getHEcmd(radioIndex, WL_HE_CMD_FEATURES, sizeof(uint32), (void *)(&features));
		if (ret < 0) {
			WIFI_ERR("%s: wl_getHEcmd Failed. radioIndex=%d, ret=%d\n",
					__FUNCTION__, radioIndex, ret);
			return -1;
		}

		if (cmd & CMD_GET) {
			*mutype = hefeatures2mutype(features, mu_features, direction);
			*plen = sizeof(*mutype);
		}

                if (cmd & CMD_SET_NVRAM) {
                        if (hemutype2features(*mutype, &features) < 0) {
                                WIFI_ERR("%s: hemutype2features mutyp=%d features=%d failed\n", __FUNCTION__, *mutype, (int)features);
                                return -2;
                        }
                        NVRAM_INT_SET(nvifname, "he_features", features == HE_FEATURES_DEFAULT ? -1 : features);
                        WIFI_DBG("%s: commit %s=0x%x to nvram\n", __FUNCTION__,
                                        nvram_name, features == HE_FEATURES_DEFAULT ? -1 : features);
                }

                if (cmd & CMD_SET_IOCTL) {
			if (hemutype2features(*mutype, &features) < 0) {
				WIFI_ERR("%s: hemutype2features mutyp=%d features=%d failed\n", __FUNCTION__, *mutype, (int)features);
				return -2;
			}
			if (wl_setHEcmd(radioIndex, WL_HE_CMD_FEATURES, sizeof(features),
						(void *)&features) < 0) {
				WIFI_ERR("%s: wl_setHEcmd he features 0x%x Failed\n",
						__FUNCTION__, features);
				return -3;
			}

			enable = (features != 0) ? 1 : 0;
			if (wl_setHEcmd(radioIndex, WL_HE_CMD_ENAB, sizeof(features), &enable) < 0) {
				WIFI_ERR("%s: wl_setHEcmd he %d Failed\n", __FUNCTION__, enable);
				return -3;
			}
			WIFI_DBG("%s: send he feature 0x%d to driver completed\n", __FUNCTION__, (int)features);
		}
	}

	if (cmd & CMD_GET_NVRAM) {
		NVRAM_INT_GET(nvifname, "he_features", &features);
		/* when nvram wlx_he_features is missing start from default */
		features = (!features) ? HE_FEATURES_DEFAULT : features;
		if (cmd & CMD_GET_NVRAM) {
			*mutype = hefeatures2mutype(features, mu_features, direction);
			*plen = sizeof(*mutype);
		}
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
				X_BROADCOM_COM_Radio_AxMuType_MASK);
		if (pObj == NULL)
			return -1;

		if (*mutype > HE_MU_UL_OFDMA) {
			pObj->reject_map |= X_BROADCOM_COM_Radio_AxMuType_MASK;
			WIFI_ERR("%s: %d is not valid he mu type\n", __FUNCTION__, *mutype);
			wldm_rel_Object(pObj, FALSE);
			return -2;
		}
		pObj->X_BROADCOM_COM_Radio.AxMuType = (uint32)*mutype;
		pObj->apply_map |= X_BROADCOM_COM_Radio_AxMuType_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	return 0;
}

int
wldm_xbrcm_Radio_AXmuEdca(int cmd, int radioIndex, wldm_wifi_edca_t *wldm_edca, uint *plen,
	char *pbuf, int *pbufsz)
{
	char *osifname = wldm_get_radio_osifname(radioIndex);
	char *parameter = "HeMuEdcs";
	wl_he_muedca_v1_t muedca;
	int ret, aci = 0;

	IGNORE_CMD_WARNING(cmd, CMD_ADD | CMD_DEL);

	if (cmd == CMD_LIST) {
		/* List the name only. */
		PRINT_BUF(pbuf, *pbufsz, "%s\n", parameter);
		return 0;
	}

	if (is11AXCapable(osifname) < 0) {
		WIFI_ERR("%s: failed to get wifi capabilities\n", __FUNCTION__);
		return -1;
	}

	if (cmd & CMD_GET) {
		ret = wl_getHEcmd(radioIndex, WL_HE_CMD_MUEDCA, sizeof(muedca), (void *)(&muedca));
		if (ret < 0) {
			WIFI_ERR("%s: ERROR getting data from MUEDCA iovar\n", __FUNCTION__);
			return -1;
		}
		if (muedca.version != WL_HE_VER_1) {
			WIFI_ERR("%s: ERROR wrong version of muedca structure %d\n",
				__FUNCTION__, muedca.version);
			return -1;
		}
		aci = wldm_edca->aci;
		wldm_edca->aifsn = muedca.ac_param_sta[aci].aci_aifsn & EDCF_AIFSN_MASK;
		wldm_edca->ecw_min = muedca.ac_param_sta[aci].ecw_min_max & EDCF_ECWMIN_MASK;
		wldm_edca->ecw_max = (muedca.ac_param_sta[aci].ecw_min_max & EDCF_ECWMAX_MASK) >>
					EDCF_ECWMAX_SHIFT;
		wldm_edca->timer = muedca.ac_param_sta[aci].muedca_timer;
	}

	if (cmd & CMD_SET) {
		X_BROADCOM_COM_Radio_Object *pObj = wldm_get_X_BROADCOM_COM_RadioObject(radioIndex,
			X_BROADCOM_COM_Radio_AxMuEdca_MASK);
		if (pObj == NULL)
			return -1;

		ret = 0;
		if ((wldm_edca->aci < AC_BE) || (wldm_edca->aci >= AC_COUNT)) {
			WIFI_ERR("%s: Wrong ACI parameter %d\n", __FUNCTION__, wldm_edca->aci);
			pObj->reject_map |= X_BROADCOM_COM_Radio_AxMuEdca_MASK;
			ret = -1;
		} else if ((wldm_edca->aifsn < EDCF_AIFSN_MIN) || (wldm_edca->aifsn > EDCF_AIFSN_MAX)) {
			WIFI_ERR("%s: Wrong AIFSN parameter %d\n", __FUNCTION__, wldm_edca->aifsn);
			pObj->reject_map |= X_BROADCOM_COM_Radio_AxMuEdca_MASK;
			ret = -1;
		} else if ((wldm_edca->ecw_min < EDCF_ECW_MIN) || (wldm_edca->ecw_min > EDCF_ECW_MAX)) {
			WIFI_ERR("%s: Wrong ECW_MIN parameter %d\n", __FUNCTION__, wldm_edca->ecw_min);
			pObj->reject_map |= X_BROADCOM_COM_Radio_AxMuEdca_MASK;
			ret = -1;
		} else if ((wldm_edca->ecw_max < EDCF_ECW_MIN) || (wldm_edca->ecw_max > EDCF_ECW_MAX)) {
			WIFI_ERR("%s: Wrong ECW_MAX parameter %d\n", __FUNCTION__, wldm_edca->ecw_max);
			pObj->reject_map |= X_BROADCOM_COM_Radio_AxMuEdca_MASK;
			ret = -1;
		} else if ((wldm_edca->timer < 0) || (wldm_edca->timer > 255)) {
			WIFI_ERR("%s: Wrong timer value %d\n", __FUNCTION__, wldm_edca->timer);
			pObj->reject_map |= X_BROADCOM_COM_Radio_AxMuEdca_MASK;
			ret = -1;
		}
		if (ret < 0) {
			wldm_rel_Object(pObj, FALSE);
			return -1;
		}

		pObj->X_BROADCOM_COM_Radio.AxMuEdca.aci = wldm_edca->aci;
		pObj->X_BROADCOM_COM_Radio.AxMuEdca.aifsn = wldm_edca->aifsn;
		pObj->X_BROADCOM_COM_Radio.AxMuEdca.ecw_min = wldm_edca->ecw_min;
		pObj->X_BROADCOM_COM_Radio.AxMuEdca.ecw_max = wldm_edca->ecw_max;
		pObj->X_BROADCOM_COM_Radio.AxMuEdca.timer = wldm_edca->timer;
		pObj->apply_map |= X_BROADCOM_COM_Radio_AxMuEdca_MASK;
		wldm_rel_Object(pObj, TRUE);
	}

	if (cmd & CMD_SET_IOCTL) {
		ret = wl_getHEcmd(radioIndex, WL_HE_CMD_MUEDCA, sizeof(muedca), (void *)(&muedca));
		if (ret < 0) {
			WIFI_ERR("%s: ERROR getting data from MUEDCA iovar\n", __FUNCTION__);
			return -1;
		}
		if (muedca.version != WL_HE_VER_1) {
			WIFI_ERR("%s: ERROR wrong version of muedca structure %d\n",
				__FUNCTION__, muedca.version);
			return -1;
		}
		aci = wldm_edca->aci;
		muedca.ac_param_sta[aci].aci_aifsn &= ~EDCF_AIFSN_MASK;
		muedca.ac_param_sta[aci].aci_aifsn |= wldm_edca->aifsn;
		muedca.ac_param_sta[aci].ecw_min_max = (wldm_edca->ecw_max << EDCF_ECWMAX_SHIFT) |
							wldm_edca->ecw_min;
		muedca.ac_param_sta[aci].muedca_timer = wldm_edca->timer;
		ret = wl_setHEcmd(radioIndex, WL_HE_CMD_MUEDCA, sizeof(muedca), (void *)&muedca);
		if (ret < 0) {
			WIFI_ERR("%s: wl_setHEcmd he muedcs Failed\n", __FUNCTION__);
			return -1;
		}
	}
	return 0;
}

/* Assumes that wldm_channelMap_ptr states are partially set from reading per_chan_info */
static int
wl_set_dfs_ch_state(uint channel, uint32 dfs_cac_state, wldm_channelMap_t *wldm_channelMap_ptr,
	uint num_channels)
{
	int i;
	wldm_channelState_t ch_state;

	if (!channel) {
		/* Channel can be zero, for example, any ext80[4] in 80MHz channel */
		WIFI_DBG("%s: Sub band channel %d not defined\n", __FUNCTION__, channel);
		return 0;
	}
	for (i = 0; i < num_channels; i++) {
		if (channel == wldm_channelMap_ptr[i].ch_number) {
			ch_state = (dfs_cac_state == WL_DFS_CACSTATE_POSTISM_OOC) ?
				WLDM_CHAN_STATE_DFS_NOP_FINISHED :
				(dfs_cac_state == WL_DFS_CACSTATE_PREISM_OOC) ?
				WLDM_CHAN_STATE_DFS_NOP_START :
				(dfs_cac_state == WL_DFS_CACSTATE_POSTISM_CAC) ?
				WLDM_CHAN_STATE_DFS_CAC_COMPLETED :
				(dfs_cac_state == WL_DFS_CACSTATE_PREISM_CAC) ?
				WLDM_CHAN_STATE_DFS_CAC_START :
				(dfs_cac_state == WL_DFS_CACSTATE_ISM) ?
				WLDM_CHAN_STATE_DFS_CAC_COMPLETED :
				WLDM_CHAN_STATE_AVAILABLE;
			wldm_channelMap_ptr[i].ch_state = ch_state;
			return 0;
		}
	}
	/* Handle unexpected unexpected channel number, log for now */
	WIFI_DBG("%s: Channel %d not found in wldm_channelMap\n", __FUNCTION__, channel);
	return -1;
}

static int
dm_channels_states(int cmd, int radioIndex, void *pvalue, uint *plen, char *pvar)
{
	int ret = 0, i;
	uint num_channels = *plen, channels_buf[WL_NUMCHANNELS + 1] = {0};
	wldm_channelMap_t *wldm_channelMap_ptr = (wldm_channelMap_t *) pvalue;
	char *osifname = wldm_get_radio_osifname(radioIndex);
	char chan_info_buf[BUF_SIZE] = {0};
	wl_uint32_list_t *list;
	wl_dfs_status_all_t *dfs_status_all_buf = NULL;

	if (strcmp(pvar, "channels_states") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;
	}

	if (cmd & CMD_GET) {

		/* Get per_chan_info and transplate into wldm_ChannelState */
		list = (wl_uint32_list_t *)channels_buf;
		list->count = htod32(WL_NUMCHANNELS);
		if (wl_ioctl(osifname, WLC_GET_VALID_CHANNELS, channels_buf, sizeof(channels_buf)) < 0) {
			WIFI_ERR("%s: IOVAR to get valid channels failed\n", __FUNCTION__);
			return -1;
		}
		list->count = dtoh32(list->count);
		if (!list->count) {
			WIFI_ERR("%s: number of valid channels is 0\n", __FUNCTION__);
			return -1;
		} else if (list->count > num_channels) {
			WIFI_ERR("%s: queried channel list count (%d) > input num_channels(%d)\n",
				__FUNCTION__, list->count, num_channels);
			return -1;
		}

		for (i = 0; i < list->count; i++) {
			uint32 chspec_info = 0;
			wldm_channelMap_ptr[i].ch_number = (uint) list->element[i];

			ret = wl_get_per_chan_info(osifname, (uint) list->element[i], chan_info_buf, BUF_SIZE);
			if (ret < 0) {
				WIFI_ERR("%s: IOVAR to get per chan info failed\n", __FUNCTION__);
				return -1;
			}
			chspec_info = dtoh32(*(uint32 *)chan_info_buf);
			if (chspec_info & WL_CHAN_RADAR) {
				if (chspec_info & WL_CHAN_INACTIVE) {
					wldm_channelMap_ptr[i].ch_state = WLDM_CHAN_STATE_DFS_NOP_START;
				} else if (!(chspec_info & WL_CHAN_PASSIVE)) {
					wldm_channelMap_ptr[i].ch_state = WLDM_CHAN_STATE_DFS_CAC_COMPLETED;
				} else if (chspec_info & WL_CHAN_PASSIVE) {
					wldm_channelMap_ptr[i].ch_state = WLDM_CHAN_STATE_DFS_NOP_FINISHED;
				}
			} else {
				wldm_channelMap_ptr[i].ch_state = WLDM_CHAN_STATE_AVAILABLE;
			}
		}

		/* Get dfs_status_all and transplate into wldm_ChannelState */
		dfs_status_all_buf = malloc(BUF_SIZE);
		if (dfs_status_all_buf == NULL) {
			WIFI_ERR("%s: Fail to alloc dfs_status_all [%d] \n", __FUNCTION__, BUF_SIZE);
			return -1;
		}
		memset(dfs_status_all_buf, 0, BUF_SIZE);
		ret = wl_get_dfs_status_all(osifname, dfs_status_all_buf, BUF_SIZE);
		if (ret < 0) {
			free(dfs_status_all_buf);
			WIFI_ERR("%s: IOVAR to get dfs status all failed\n", __FUNCTION__);
			return -1;
		}

		for (i = 0; i < dfs_status_all_buf->num_sub_status; ++i) {
			wl_dfs_sub_status_t *sub = NULL;
			wldm_channel_t chan;
			sub = &dfs_status_all_buf->dfs_sub_status[i];
			memset(&chan, 0, sizeof(wldm_channel_t));
			wl_parse_chanspec(sub->chanspec, &chan);
			/* Set ch_state from wl dfs_status_all */
			wl_set_dfs_ch_state(chan.control, sub->state, wldm_channelMap_ptr,
				num_channels);
			wl_set_dfs_ch_state(chan.ext20, sub->state, wldm_channelMap_ptr,
				num_channels);
			wl_set_dfs_ch_state(chan.ext40[0], sub->state, wldm_channelMap_ptr,
				num_channels);
			wl_set_dfs_ch_state(chan.ext40[1], sub->state, wldm_channelMap_ptr,
				num_channels);
			wl_set_dfs_ch_state(chan.ext80[0], sub->state, wldm_channelMap_ptr,
				num_channels);
			wl_set_dfs_ch_state(chan.ext80[1], sub->state, wldm_channelMap_ptr,
				num_channels);
			wl_set_dfs_ch_state(chan.ext80[2], sub->state, wldm_channelMap_ptr,
				num_channels);
			wl_set_dfs_ch_state(chan.ext80[3], sub->state, wldm_channelMap_ptr,
				num_channels);
		}

		free(dfs_status_all_buf);
	}

	return ret;
}

static xbrcm_t xbrcm_opensync_tbl[] = {
	{  "channels_states",	{dm_channels_states},	CMD_GET,			},
	{  NULL,		{NULL},			0,				},
};

int
wldm_xplume_opensync(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);

	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_opensync_tbl);
}

static int
dm_band(int cmd, int index, void *pvalue, uint *plen, char *pvar)
{
	char *osifname, *nvifname, buf[BUF_SIZE];
	int wlband = 0;
	wldm_freq_bands_t band = 0;

	if (pvalue == NULL) {
		WIFI_ERR("%s: Null pvalue\n", __FUNCTION__);
		return -1;
	}

	nvifname = wldm_get_radio_nvifname(index);
	osifname = wldm_get_radio_osifname(index);

	if (cmd & CMD_GET) {
		if (wl_ioctl(osifname, WLC_GET_BAND, &wlband, sizeof(wlband)) < 0) {
			WIFI_ERR("%s: IOCTL to get band info failed idx = %d\n", __FUNCTION__, index);
			return -1;
		}
		wlband = dtoh32(wlband);
		if (wlband == WLC_BAND_AUTO) {
			/* convert to specific operating band */
			chanspec_t chanspec;

			if (wl_iovar_getint(osifname, "chanspec", (int*)&chanspec) < 0) {
				WIFI_ERR("%s: wl_iovar_getint chanspec failed\n", __FUNCTION__);
				return -1;
			}
			wlband = CHSPEC_IS2G(chanspec) ? WLC_BAND_2G :
				(CHSPEC_IS5G(chanspec) ? WLC_BAND_5G : WLC_BAND_6G);
		}
		band = (wlband == WLC_BAND_2G) ? WLDM_FREQUENCY_2_4_BAND :
			((wlband == WLC_BAND_5G) ? WLDM_FREQUENCY_5_BAND :
			((wlband == WLC_BAND_6G) ? WLDM_FREQUENCY_6_BAND : 0));

		memcpy((char *)(pvalue), (char *)(&band), sizeof(band));
		*plen = sizeof(band);
	}

	if (cmd & CMD_SET_NVRAM) {
		char nvram_name[NVRAM_NAME_SIZE];

		wlband = *(int *)pvalue;
		wlband = ((wlband == WLDM_FREQUENCY_2_4_BAND) ? WLC_BAND_2G :
				  (wlband == WLDM_FREQUENCY_5_BAND) ? WLC_BAND_5G :
				  (wlband == WLDM_FREQUENCY_6_BAND) ? WLC_BAND_6G : 0);
		if (!wlband) {
			WIFI_ERR("%s: Invalid band!\n", __FUNCTION__);
			return -1;
		}
		snprintf(nvram_name, sizeof(nvram_name), "%s_nband", nvifname);
		snprintf(buf, sizeof(buf), "%d", wlband);
		if (nvram_set(nvram_name, buf)) {
			WIFI_ERR("%s: wlcsm_nvram_set %s=%s failed!\n", __FUNCTION__, nvram_name, buf);
			return -1;
		}
	}

	return 0;
}

static int
dm_bands(int cmd, int index, void *pvalue, uint *plen, char *pvar)
{
	char *osifname;
	int ret, i, list[4];
	wldm_freq_bands_t bands = 0;

	if (pvalue == NULL) {
		WIFI_ERR("%s: Null pvalue\n", __FUNCTION__);
		return -1;
	}
	osifname = wldm_get_radio_osifname(index);
	ret = wl_ioctl(osifname, WLC_GET_BANDLIST, list, sizeof(list));
	if (ret < 0) {
		WIFI_ERR("%s: wl_ioctl() WLC_GET_VALID_CHANNELS failed! ret=%d\n",
			__FUNCTION__, ret);
		return -1;
	}
	/* count */
	list[0] = dtoh32(list[0]);
	for (i = 1; i <= list[0]; i++) {
		list[i] = dtoh32(list[i]);
		if (list[i] == WLC_BAND_2G)
			bands |= WLDM_FREQUENCY_2_4_BAND;
		else if (list[i] == WLC_BAND_5G)
			bands |= WLDM_FREQUENCY_5_BAND;
		else if (list[i] == WLC_BAND_6G)
			bands |= WLDM_FREQUENCY_6_BAND;
	}
	if (*plen < sizeof(bands)) {
		WIFI_ERR("%s: buffer too short need %d *plen = %d\n",
			__FUNCTION__, sizeof(bands), *plen);
		return -1;
	}
	memcpy((char *)(pvalue), (char *)(&bands), sizeof(bands));
	*plen = sizeof(bands);
	return 0;
}

/* Returns number of channels and chList or -1 if IOCTL failure or buffer too short
 * - used by wifi_getRadioCapabilities
 */
static int
dm_possibleChannels(int cmd, int index, void *pvalue, uint *plen, char *pvar)
{
	char *osifname;
	wl_uint32_list_t *list = NULL;
	wldm_uint32_list_t *clist = (wldm_uint32_list_t *)(pvalue);
	uint32 possibleChannels[WL_NUMCHANNELS + 1] = {0};
	int i, ret, listlen = 0;

	if (pvalue == NULL) {
		WIFI_ERR("%s: Null pvalue\n", __FUNCTION__);
		return -1;
	}
	osifname = wldm_get_radio_osifname(index);
	list = (wl_uint32_list_t *)possibleChannels;
	list->count = htod32(WL_NUMCHANNELS);
	ret = wl_ioctl(osifname, WLC_GET_VALID_CHANNELS, possibleChannels,
			sizeof(possibleChannels));
	if (ret < 0) {
		WIFI_ERR("%s: wl_ioctl() WLC_GET_VALID_CHANNELS failed! ret=%d\n",
			__FUNCTION__, ret);
		return -1;
	}
	list->count = dtoh32(list->count);
	listlen = sizeof(list->count) + (list->count) * (sizeof(list->element[0]));
	if (*plen < listlen) {
		WIFI_ERR("%s: buffer too short need %d *plen = %d\n",
			__FUNCTION__, listlen, *plen);
		return -1;
	}
	clist->count = list->count;
	for (i = 0; i < list->count; i++) {
		clist->element[i] =  dtoh32(list->element[i]);
	}
	*plen = listlen;

	return 0;
}

static int
chanspec2band(chanspec_t chanspec, char *band, int size)
{
	char *b;

	b = (CHSPEC_IS2G(chanspec) ? "2g" :
#ifdef CHSPEC_IS6G
			CHSPEC_IS6G(chanspec) ? "6g" :
#endif /* CHSPEC_IS6G */
			CHSPEC_IS5G(chanspec) ? "5g" : "");

	if (strlen(b) >= size) {
		WIFI_DBG("%s: buffer too small %d\n", __FUNCTION__, size);
		return -1;
	}

	snprintf(band, size, "%s", b);
	WIFI_DBG("%s: band=%s\n", __FUNCTION__, band);
	return 0;
}

static int
chanspec2secondaryChannel(chanspec_t chanspec, int *num, int *chanlist, int chanlistsz)
{
	int channel, ctl_channel, i = 0;

	*num = 0;
	ctl_channel = wf_chspec_ctlchan(chanspec);

	FOREACH_20_SB(chanspec, channel) {
		if (channel != ctl_channel) {
			chanlist[i++] = channel;
			if (i >= chanlistsz)
				break;

		}
	}
	*num = i;

	WIFI_DBG("%s: control channel=%d\n", __FUNCTION__, ctl_channel);
	WIFI_DBG("%s: numSecondaryChannels=%d\n", __FUNCTION__, *num);
	WIFI_DBG("channelSecondary:\n");
	for (i = 0; i < *num; i++) {
		WIFI_DBG("channelSecondary[%d]=%d ", i, chanlist[i]);
	}
	WIFI_DBG("\n");
	return 0;
}

/* can be expanded to support other chanspec parameters */
static int
wl_get_info_from_chanspec(chanspec_t chanspec, wldm_xbrcm_radio_param_t *param)
{
	param->channel = wf_chspec_ctlchan(chanspec);
	if (chanspec2bandwidth(chanspec, param->bandwidth, sizeof(param->bandwidth)) != 0) {
		WIFI_ERR("%s: failed to parse bandwidth info from chanspec 0x%x\n",
			__FUNCTION__, chanspec);
		return -1;
	}

	if (chanspec2band(chanspec, param->band, sizeof(param->band)) != 0) {
		WIFI_ERR("%s: failed to parse band info from chanspec 0x%x\n",
			__FUNCTION__, chanspec);
		return -2;
	}

	memset(param->channelSecondary, 0, sizeof(param->channelSecondary));
	chanspec2secondaryChannel(chanspec, (int *)&(param->numSecondaryChannels),
			(int *)param->channelSecondary, ARRAY_SIZE(param->channelSecondary));
	return 0;
}

static int
dm_chanspec(int cmd, int radioIndex, wldm_xbrcm_radio_param_t *val, uint *plen, char *pvar)
{
	int chanspec;
	char *osifname = wldm_get_radio_osifname(radioIndex);

	if (strcmp(pvar, "chanspec") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar);
		return -1;
	}

	if (!val) {
		WIFI_ERR("%s: wldm_xbrcm_radio_param_t buf not allocated\n", __FUNCTION__);
		return -1;
	}

	memset(val, 0, sizeof(wldm_xbrcm_radio_param_t));
	if (wl_iovar_getint(osifname, "chanspec", (int*)&chanspec) < 0) {
		WIFI_ERR("%s: wl_iovar_getint chanspec failed\n", __FUNCTION__);
		return -1;
	}

	if (cmd & CMD_GET) {
		if (wl_get_info_from_chanspec((chanspec_t)chanspec, val) != 0) {
			WIFI_ERR("%s: wl_get_chanspec failed \n", __FUNCTION__);
			return -1;
		}
		WIFI_DBG("%s: get: chanspec = %x ch = %d bw = %s band = %s numSecond = %d\n",
			__FUNCTION__, chanspec, val->channel, val->bandwidth, val->band,
				val->numSecondaryChannels);
	}

	return 0;
}

typedef struct _map_tbl {
	int var1;
	int var2;
} map_tbl_t;

typedef enum _wldm_guardinterval {
	wldm_guard_interval_400 =	0x01,
	wldm_guard_interval_800 =	0x02,
	wldm_guard_interval_1600 =	0x04,
	wldm_guard_interval_3200 =	0x08,
	wldm_guard_interval_auto =	0x10,
} wldm_guardinterval_t;

#define WL_RSPEC_GI_AUTO	0
#define HEGI_OFFSET		2           /* skip the legacy sgi */
static map_tbl_t wifi_guardinterval_Infotbl[] = {
	{WL_RSPEC_GI_AUTO,		wldm_guard_interval_auto},	/* hegi=0, sgi=0,  wldmgi=0x10 all */
	{WL_RSPEC_SGI,			wldm_guard_interval_400},	/* hegi=0, sgi=1, wldmgi=0x01 none he */
	{WL_RSPEC_HE_1x_LTF_GI_0_8us,	wldm_guard_interval_800},	/* hegi=0, sgi=0, wldmgi=0x02 HE_1x_LTF */
	{WL_RSPEC_HE_2x_LTF_GI_0_8us,	wldm_guard_interval_800},	/* hegi=1, sgi=0, wldmgi=0x02 HE_2x_LTF */
	{WL_RSPEC_HE_2x_LTF_GI_1_6us,	wldm_guard_interval_1600},	/* hegi=2, sgi=0, wldmgi=0x04 HE_3x_LTF */
	{WL_RSPEC_HE_4x_LTF_GI_3_2us,	wldm_guard_interval_3200}	/* hegi=3, sgi=0, wldmgi=0x08 HE_4x_LTF */
};

static int
get_var1(int *var1, int var2, map_tbl_t *tbl, int tbl_sz, uint offset)
{
	int i;

	if (!tbl || !var1)
		return -1;

	*var1 = 0;
	for (i = offset; i < tbl_sz; i++) {
		if (tbl[i].var2 == var2) {
			*var1 = tbl[i].var1;
			return 0;
		}
	}
	return -1;
}

static int
get_var2(int var1, int *var2, map_tbl_t *tbl, int tbl_sz, uint offset)
{
	int i;

	if (!tbl || !var2)
		return -1;

	*var2 = 0;
	for (i = offset; i < tbl_sz; i++) {
		if (tbl[i].var1 == var1) {
			*var2 = tbl[i].var2;
			return 0;
		}
	}
	return -1;
}

static int
dm_guardInterval(int cmd, int radioIndex, wldm_guardinterval_t *pvalue, int *plen, char *pvar)
{
	unsigned int rspec = 0, isHeGI = 0, is2xLTF = 0, offset = 0, encode = 0, bw = 0;
	int gi = 0, band = 0, ret;
	char *osifname = wldm_get_radio_osifname(radioIndex);
	char *iovar = NULL;

	if (!pvar || strcmp(pvar, "guardInterval") != 0) {
		WIFI_ERR("%s: invalid pvar %s\n", __FUNCTION__, pvar ? pvar : "null");
		return -1;
	}

	if (!pvalue) {
		WIFI_ERR("%s: invalid pvalue\n", __FUNCTION__);
		return -1;
	}

	if (wl_ioctl(osifname, WLC_GET_BAND, &band, sizeof(band)) < 0) {
		WIFI_ERR("%s: wl_ioctl WLC_GET_BAND failed!\n", __FUNCTION__);
		return -1;
	}
	band = dtoh32(band);
	if (band == WLC_BAND_AUTO) {
		/* convert to specific operating band */
		chanspec_t chanspec;

		if (wl_iovar_getint(osifname, "chanspec", (int*)&chanspec) < 0) {
			WIFI_ERR("%s: wl_iovar_getint chanspec failed\n", __FUNCTION__);
			return -1;
		}
		band = CHSPEC_IS2G(chanspec) ? WLC_BAND_2G :
			(CHSPEC_IS5G(chanspec) ? WLC_BAND_5G : WLC_BAND_6G);
	}
	iovar = ((band == WLC_BAND_2G) ? "2g_rate" :
			(band == WLC_BAND_5G) ? "5g_rate" :
			(band == WLC_BAND_6G) ? "6g_rate" : NULL);

	if (!iovar) {
		WIFI_ERR("%s: invalid band %d\n", __FUNCTION__, band);
		return -1;
	}
	WIFI_DBG("%s: iovar=%s\n", __FUNCTION__, iovar);

	if (wl_iovar_getint(osifname, "nrate", (int *) &rspec) < 0) {
		WIFI_ERR("%s: wl_iovar_getint() %s failed!\n", __FUNCTION__, iovar);
		return -1;
	}
	encode = (rspec & WL_RSPEC_ENCODING_MASK);
	isHeGI = (encode == WL_RSPEC_ENCODE_HE) ? 1 : 0;
	if (isHeGI) {
		gi |= RSPEC_HE_LTF_GI(rspec);
		is2xLTF = HE_IS_2X_LTF(gi);
		bw = RSPEC_BW(rspec);
	}

	WIFI_DBG("%s: %s %s return rspec 0x%x encode=0x%x\n",
			__FUNCTION__, osifname, iovar, rspec, encode);

	if (cmd & CMD_GET) {
		if (isHeGI) {
			offset = HEGI_OFFSET;
		} else if (RSPEC_ISSGI(rspec)) {
			gi |= WL_RSPEC_SGI;
		}
		ret = get_var2(gi, (int *)pvalue, wifi_guardinterval_Infotbl,
			ARRAY_SIZE(wifi_guardinterval_Infotbl), offset);

		if (ret != 0) {
			WIFI_ERR("%s: 0x%x is invalid guardinterval from driver\n",
					__FUNCTION__, gi);
			return -1;
		}
		if (!(rspec & (WL_RSPEC_OVERRIDE_RATE | WL_RSPEC_OVERRIDE_MODE)))
			*pvalue = wldm_guard_interval_auto;

		WIFI_DBG("%s: isHeGI=%d  isSGI=0x%x gi=0x%x *pvalue=0x%x\n",
			       __FUNCTION__, isHeGI, RSPEC_ISSGI(rspec), gi, *pvalue);
	}

	if (cmd & CMD_SET_IOCTL) {
		WIFI_DBG("%s: wldm_guardinterval_t *pvalue=0x%x\n", __FUNCTION__, *pvalue);
		if ((*pvalue) == wldm_guard_interval_auto) {
			rspec = 0;
			WIFI_DBG("%s: wldm_guard_interval_auto, rspec=%d\n", __FUNCTION__, rspec);
		} else {
			isHeGI = ((*pvalue) >= wldm_guard_interval_800) &&
				((*pvalue) <= wldm_guard_interval_3200) ? 1 : 0;
			offset = isHeGI ? HEGI_OFFSET : 0;
			ret = get_var1(&gi, *(int *)pvalue, wifi_guardinterval_Infotbl,
					ARRAY_SIZE(wifi_guardinterval_Infotbl), offset);
			if (ret != 0) {
				WIFI_ERR("%s: 0x%x is invalid argument for *pvalue\n",
						__FUNCTION__, *pvalue);
				return -1;
			}
			if (*pvalue == wldm_guard_interval_800) {
				gi = (is2xLTF ? WL_RSPEC_HE_2x_LTF_GI_0_8us :
						WL_RSPEC_HE_1x_LTF_GI_0_8us);
			}
			WIFI_DBG("%s: isHeGI=%d table offset=%d gi=0x%x\n",
					__FUNCTION__, isHeGI, offset, gi);

			if (isHeGI) {
				rspec &= (~WL_RSPEC_SGI);
				rspec &= (~WL_RSPEC_HE_GI_MASK);
				rspec |= HE_GI_TO_RSPEC(gi);
				if (bw > WL_RSPEC_BW_20MHZ) {
					/* Set LDPC for HE > 20MHz */
					rspec |= WL_RSPEC_LDPC;
				}
			} else {
				rspec &= (~WL_RSPEC_HE_GI_MASK);
				rspec &= (~WL_RSPEC_SGI);
				if ((*pvalue) == wldm_guard_interval_400) {
					rspec |= WL_RSPEC_SGI;
				}
			}

			/* set mode override */
			rspec |= WL_RSPEC_OVERRIDE_MODE;
		}

		WIFI_DBG("%s: rspec=0x%x\n", __FUNCTION__, rspec);
		ret = wl_iovar_setint(osifname, iovar, rspec);
		if (ret < 0) {
			WIFI_DBG("%s: wl_iovar_setint %s ret=%d\n", __FUNCTION__, iovar, ret);
			return -1;
		}
	}

	return 0;
}

static int
dm_countryList(int cmd, int radioIndex, wl_country_list_t *pvalue, int *plen, char *pvar)
{
	char buf[WLC_IOCTL_MEDLEN] = {0}, tmp_abbrev[WLC_CNTRY_BUF_SZ] = {0};
	wl_country_t country_spec = {{0}, 0, {0}};
	wl_country_list_t *cl = (wl_country_list_t *)buf;
	char *abbrev, *curr_abbrev, *first_abbrev, *osifname = wldm_get_radio_osifname(radioIndex);
	int i, len, ret;

	ret = wl_iovar_get(osifname, "country", &country_spec, sizeof(country_spec));
	if (ret < 0) {
		WIFI_ERR("%s: Error get country ret=%d\n", __FUNCTION__, ret);
		return -1;
	}
	curr_abbrev = country_spec.country_abbrev;

	cl->buflen = sizeof(buf);
	ret = wl_ioctl(osifname, WLC_GET_COUNTRY_LIST, buf, sizeof(buf));
	if (ret < 0) {
		WIFI_ERR("%s: Error WLC_GET_COUNTRY_LIST ret=%d\n", __FUNCTION__, ret);
		return -1;
	}
	WIFI_DBG("%s: cl_cnt=%d\n", __FUNCTION__, cl->count);
	len = sizeof(wl_country_list_t) + (cl->count * WLC_CNTRY_BUF_SZ);
	if (*plen < len) {
		WIFI_ERR("%s: Error Buffer too short need %d got %d\n", __FUNCTION__, len, *plen);
		return -1;
	}

	first_abbrev = &cl->country_abbrev[0];
	for (i = 0; i < cl->count; i++) {
		abbrev = &cl->country_abbrev[i*WLC_CNTRY_BUF_SZ];
		if (!(strncmp(abbrev, curr_abbrev, WLC_CNTRY_BUF_SZ))) {
			/* found curr_abbrev in list */
			if (i > 0) {
				/* swap to make curr ccode first */
				strncpy(tmp_abbrev, first_abbrev, WLC_CNTRY_BUF_SZ);
				strncpy(first_abbrev, abbrev, WLC_CNTRY_BUF_SZ);
				strncpy(abbrev, tmp_abbrev, WLC_CNTRY_BUF_SZ);
			}
			WIFI_DBG("%s aft-SWAP i=%d curr=%s first=%s abbrev=%s\n", __FUNCTION__, i,
				curr_abbrev, first_abbrev, abbrev);
			break;
		}
	}

	memcpy((char *)(pvalue), (char *)cl, len);
	*plen = len;

	return 0;
}

static xbrcm_t xbrcm_radio_tbl[] = {
	{  "band",			{dm_band},		CMD_GET | CMD_SET_NVRAM,	},
	{  "bands",			{dm_bands},		CMD_GET,	},
	{  "possibleChannels",		{dm_possibleChannels},	CMD_GET,	},
	{  "chanspec",			{dm_chanspec},		CMD_GET,	},
	{  "guardInterval",		{dm_guardInterval},     CMD_GET | CMD_SET_IOCTL,    },
	{  "countryList",		{dm_countryList},	CMD_GET,	},
	{  NULL,			{NULL},			0,		},
};

int
wldm_xbrcm_radio(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);
	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_radio_tbl);
}

static int
dm_atm_enable(int cmd, int radioIndex, int *pvalue, uint *plen, char *pvar)
{
	char *osifname = wldm_get_radio_osifname(radioIndex);
	char *nvifname = wldm_get_radio_nvifname(radioIndex);
	char *nvram_get_value, nvram_name[NVRAM_NAME_SIZE], nvram_set_value[BUF_SIZE];
	int atf_status;

	if (cmd & CMD_GET) {
		if (wl_iovar_getint(osifname, "atf", &atf_status) < 0) {
			WIFI_ERR("%s: wl_iovar_getint(atf) failed!\n", __FUNCTION__);
			return -1;
		}
		*pvalue = atf_status;
		*plen =  sizeof(atf_status);
		/* Return here to avoid confusion with another GET cmd */
		return 0;
	}
	if (cmd & CMD_SET_IOCTL) {
		if (wl_iovar_setint(osifname, "atf", *pvalue) != 0) {
			WIFI_ERR("%s: wl_iovar_setint(atf=%d) failed!\n",
				__FUNCTION__, *pvalue);
			return -1;
		}
	}
	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_atf", nvifname);
		nvram_get_value = wlcsm_nvram_get(nvram_name);
		if (nvram_get_value) {
			*pvalue = atoi(nvram_get_value);
			*plen = sizeof(*pvalue);
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get(%s) failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}
		/* Return here to avoid confusion with another GET cmd */
		return 0;
	}
	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_atf", nvifname);
		snprintf(nvram_set_value, sizeof(nvram_get_value), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, nvram_set_value) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set(%s=%s) failed!\n",
				__FUNCTION__, nvram_name, nvram_set_value);
		}
	}

	return 0;
}

static int
dm_atm_bssperc(int cmd, int apIndex, uint *pvalue, uint *plen, char *pvar)
{
	char *osifname = wldm_get_osifname(apIndex);
	char *nvifname = wldm_get_nvifname(apIndex);
	boolean enable = TRUE;
	int len = sizeof(enable);
	char nvram_name[NVRAM_NAME_SIZE], *nvram_get_value, nvram_set_value[BUF_SIZE];
	uint bssperc = *pvalue;

	if (cmd & CMD_GET) {
		if (wldm_AccessPoint_Enable(cmd, apIndex, &enable, &len, NULL, NULL) < 0) {
			WIFI_ERR("%s, wldm_AccessPoint_Enable CMD_GET failed, apIndex = %d\n",
				__FUNCTION__, apIndex);
			return -1;
		}
		if (wl_iovar_getint(osifname, "atm_bssperc", (int *) &bssperc) < 0) {
			WIFI_ERR("%s: wl_iovar_getint(atm_bssperc) failed!\n", __FUNCTION__);
			return -1;
		}
		*pvalue = bssperc;
		*plen =  sizeof(bssperc);
		/* Return here to avoid confusion with another GET cmd */
		return 0;
	}
	if (cmd & CMD_SET_IOCTL) {
		if (wldm_AccessPoint_Enable(cmd, apIndex, &enable, &len, NULL, NULL) < 0) {
			WIFI_ERR("%s, wldm_AccessPoint_Enable CMD_GET failed, apIndex = %d\n",
				__FUNCTION__, apIndex);
			return -1;
		}
		if (wl_iovar_setint(osifname, "atm_bssperc", *pvalue) != 0) {
			WIFI_ERR("%s: wl_iovar_setint(atm_bssperc=%d) failed!\n",
				__FUNCTION__, *pvalue);
			return -1;
		}
	}
	if (cmd & CMD_GET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_atm_bssperc", nvifname);
		nvram_get_value = wlcsm_nvram_get(nvram_name);
		if (nvram_get_value) {
			*pvalue = atoi(nvram_get_value);
			*plen = sizeof(*pvalue);
		} else {
			WIFI_ERR("%s: wlcsm_nvram_get(%s) failed!\n", __FUNCTION__, nvram_name);
			return -1;
		}
		/* Return here to avoid confusion with another GET cmd */
		return 0;
	}
	if (cmd & CMD_SET_NVRAM) {
		snprintf(nvram_name, sizeof(nvram_name), "%s_atm_bssperc", nvifname);
		snprintf(nvram_set_value, sizeof(nvram_get_value), "%d", *pvalue);
		if (wlcsm_nvram_set(nvram_name, nvram_set_value) != 0) {
			WIFI_ERR("%s: wlcsm_nvram_set(%s=%s) failed!\n",
				__FUNCTION__, nvram_name, nvram_set_value);
		}
	}

	return 0;
}

static int
dm_atm_staperc(int cmd, int apIndex, void *pvalue, uint *plen, char *pvar)
{
	char *osifname = wldm_get_osifname(apIndex);
	wldm_atm_staperc_t *wldm_staperc = (wldm_atm_staperc_t *) pvalue;
	char buf[WLC_IOCTL_MEDLEN] = "";
	wl_atm_staperc_t atm_staperc_buf;

	if (!_wl_ether_atoe(wldm_staperc->macstr, &atm_staperc_buf.ea)) {
		WIFI_ERR("%s: _wl_ether_atoe(%s) failed!\n", __FUNCTION__, wldm_staperc->macstr);
		return -1;
	}

	if (cmd & CMD_GET) {
		if (wl_iovar_getbuf(osifname, "atm_staperc", &atm_staperc_buf,
			sizeof(atm_staperc_buf), buf, WLC_IOCTL_MEDLEN) < 0) {
			WIFI_ERR("%s: wl_iovar_getbuf(atm_staperc) failed!\n", __FUNCTION__);
			return -1;
		}
		wldm_staperc->perc = ((wl_atm_staperc_t *) buf)->perc;
		*plen =  sizeof(*pvalue);
		/* Return here to avoid confusion with another GET cmd */
		return 0;
	}
	if (cmd & CMD_SET_IOCTL) {
		atm_staperc_buf.perc = wldm_staperc->perc;
		if (wl_iovar_setbuf(osifname, "atm_staperc", &atm_staperc_buf,
			sizeof(atm_staperc_buf), buf, WLC_IOCTL_MEDLEN) < 0) {
			WIFI_ERR("%s: wl_iovar_setbuf(atm_staperc) failed!\n", __FUNCTION__);
			return -1;
		}
	}

	return 0;
}

static xbrcm_t xbrcm_atm_tbl[] = {
	{  "enable",			{dm_atm_enable},	CMD_GET | CMD_SET_IOCTL | CMD_GET_NVRAM | CMD_SET_NVRAM	},
	{  "bssperc",			{dm_atm_bssperc},	CMD_GET | CMD_SET_IOCTL | CMD_GET_NVRAM | CMD_SET_NVRAM	},
	{  "staperc",			{dm_atm_staperc},	CMD_GET | CMD_SET_IOCTL					},
	{  NULL,			{NULL},			0							},
};

int
wldm_xbrcm_atm(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);

	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_atm_tbl);
}

/* For wifi_getApAssociatedDevicesHighWatermarkThreshold etc.
	pvar string is same as the nvram name
	all counters are calculated and saved to nvram by ecbd
 */
static int
dm_assoc_dev_hwm(int cmd, int apIndex, int *pvalue, uint *plen, char *pvar)
{
	char *nvifname = wldm_get_nvifname(apIndex);
	char nv_name[64], *nv_value = NULL, new_value[64];
	int maxassoc = 0, len, hwm_th = 0, adjust_th;

	if (!pvalue || !pvar) {
		WIFI_ERR("%s: input pointer is null\n", __FUNCTION__);
		return -1;
	}

	if (strcmp(pvar, "assoc_dev_hwm_th") == 0) {
		hwm_th = 1;
		/* read MaxAssociatedDevices first */
		len = sizeof(maxassoc);
		if (wldm_AccessPoint_MaxAssociatedDevices(CMD_GET_NVRAM, apIndex,
			&maxassoc, &len, NULL, NULL) != 0) {
			WIFI_ERR("%s: Fail to get MaxAssociatedDevices for apIndex=%d\n",
				__FUNCTION__, apIndex);
			maxassoc = 0; /* use this as default for un-config case */
		}
	}

	snprintf(nv_name, sizeof(nv_name), "%s_%s", nvifname, pvar);

	WIFI_DBG("%s: apIndex=%d ifname=%s nv_name=%s hwm_th=%d maxassoc=%d\n",
		__FUNCTION__, apIndex, nvifname, nv_name, hwm_th, maxassoc);

	if (cmd & CMD_GET_NVRAM) {
		nv_value = nvram_get(nv_name);
		WIFI_DBG("%s: get nv_name=%s value=%s\n",
			__FUNCTION__, nv_name, nv_value ? nv_value : "<null>");

		*pvalue = nv_value ? atoi(nv_value) : 0;

		/* Adjust HWM threshold per Request:
			if MaxAssociatedDevices is 0, HighWatermarkThreshold should be default
			value which is 50,
			else HighWatermarkThreshold should be less than MaxAssociatedDevices.
		*/
		if (hwm_th) {
			if (nv_value) {
				if (*pvalue > maxassoc)
					*pvalue = maxassoc;
			} else {
				/* not configured, use default */
				*pvalue = maxassoc ? maxassoc : DEFAULT_ASSOC_DEV_HWM_TH;
			}
		}

		if (plen) {
			*plen = sizeof(*pvalue);
		}
	}

	if (cmd & CMD_SET_NVRAM) {
		if (hwm_th) {
			adjust_th = *pvalue;
			/* allow "0" to turn off the calculation */
			if (adjust_th != 0) {
				if (maxassoc == 0)
					adjust_th = DEFAULT_ASSOC_DEV_HWM_TH;
				else if (*pvalue > maxassoc)
					adjust_th = maxassoc;
			}

			snprintf(new_value, sizeof(new_value), "%d", adjust_th);
			if (nvram_set(nv_name, new_value)) {
				WIFI_ERR("%s: nvram_set %s FAIL.", __FUNCTION__, nv_name);
				return -1;
			}
			WIFI_ERR("%s: nvram_set %s for %s OK",
				__FUNCTION__, new_value, nv_name);
		}
		else {
			WIFI_ERR("%s: not support nvram_set for %s", __FUNCTION__, pvar);
			return -1;
		}
	}

	return 0;
}

static xbrcm_t xbrcm_assoc_dev_hwm_tbl[] = {
	{  "assoc_dev_hwm_th",		{dm_assoc_dev_hwm},	CMD_GET_NVRAM | CMD_SET_NVRAM,	},
	{  "assoc_dev_hwm_th_reached",	{dm_assoc_dev_hwm},	CMD_GET_NVRAM,		},
	{  "assoc_dev_hwm_max",		{dm_assoc_dev_hwm},	CMD_GET_NVRAM,		},
	{  "assoc_dev_hwm_max_date",	{dm_assoc_dev_hwm},	CMD_GET_NVRAM,		},
	{  NULL,			{NULL},			0,				},
};

int
wldm_xbrcm_assoc_dev_hwm(int cmd, int index, void *pvalue, uint *plen, char *pvar, int *pvarsz)
{
	UNUSED_PARAMETER(pvarsz);
	return dm_xbrcm(cmd, index, pvalue, plen, pvar, xbrcm_assoc_dev_hwm_tbl);
}
/* End of ApAssociatedDevicesHighWatermarkThreshold */

/*----------------------------------*/
/* END */
