/*
 * NVRAM variable manipulation (common)
 *
 * Copyright (C) 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * NVRAM emulation interface
 *
 * $Id: nvram_shared.c $
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <typedefs.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <bcmutils.h>

#include <netinet/tcp.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

/* for file locking */
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* for mmap */
#include <sys/mman.h>

/*Macro Definitions*/
#define _MALLOC_(x)         calloc(x, sizeof(char))
#define _MFREE_(buf, size)  free(buf)
#define VALID_BIT(bit) ((bit) >=0 && (bit) <= 31)
#define CODE_BUFF	16
#define HEX_BASE	16

/*the following definition is from wldef.h*/
#define WL_MID_SIZE_MAX  32
#define WL_SSID_SIZE_MAX 48
#define WL_WEP_KEY_SIZE_MAX WL_MID_SIZE_MAX
#define WL_WPA_PSK_SIZE_MAX  72  // max 64 hex or 63 char
#define WL_UUID_SIZE_MAX  40

#define WL_DEFAULT_VALUE_SIZE_MAX  128
#define WL_DEFAULT_NAME_SIZE_MAX  20
#define WL_WDS_SIZE_MAX  80

/* internal structure */
static struct nvram_tuple * nvram_hash[32] = { NULL };
static int nvram_inited=0;

#define  NVRAMD_DFLT_PORT           5152           /* default nvramd port number       */

typedef struct nvramc_info {
    int conn_fd;
    char cmd_buf[MAX_NVRAM_SPACE];
    char cur_cmd[10];
    unsigned int cmd_size;
} nvramc_info_t;

nvramc_info_t nvramc_info;

/*Differennt Nvram variable has different value length. To keep the Hash table static and sequence,
when one nvrma variable is inserted into hash table, the location will not dynamic change.
This structure is used to keep nvram name and value length*/
/* When new nvram variable is defined and max length is more than WL_DEFAULT_VALUE_SIZE_MAX,
the name and max length should be added into var_len_tab*/

struct   nvram_var_len_table {
    char *name;
    unsigned int  max_len;
};

/*nvram variable vs max length table*/
struct nvram_var_len_table var_len_tab[] =
{
#ifdef NVRAM_PREFIX_PARSE
        {"ssid",     WL_SSID_SIZE_MAX+1},
        {"uuid",    WL_UUID_SIZE_MAX+1},
#else
        {"wsc_ssid",     WL_SSID_SIZE_MAX+1},
        {"wsc_uuid",    WL_UUID_SIZE_MAX+1},
        {"wps_ssid",     WL_SSID_SIZE_MAX+1},
        {"wps_uuid",    WL_UUID_SIZE_MAX+1},
#endif
        {"radius_key",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"wpa_psk",    WL_WPA_PSK_SIZE_MAX+1},
        {"key1",          WL_MID_SIZE_MAX+1 },
        {"key2",          WL_MID_SIZE_MAX+1 },
        {"key3",          WL_MID_SIZE_MAX+1 },
        {"key4",          WL_MID_SIZE_MAX+1 },
        {"wds",            WL_WDS_SIZE_MAX },
#ifdef NVRAM_PREFIX_PARSE
        {"ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
#else
        {"lan_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"lan1_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"lan2_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"lan3_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"lan4_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"br0_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"br1_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"br2_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
        {"br3_ifnames",  WL_DEFAULT_VALUE_SIZE_MAX * 3},
#endif
/*This nvram variavle is used for debug*/
        {"wldbg",            1024 },
};

#define VAR_LEN_COUNT (sizeof(var_len_tab) /sizeof(struct nvram_var_len_table))

#ifdef NVRAM_PREFIX_PARSE
/* defines the nvram prefix structure */
struct   prefix_table {
    char *name;
};

/* Defines nvram variable needing to filter out following prefix.
   Such as wl1.0_ssid, will be filtered to be ssid.

   This is a simple Pattern/Syntax mapping case.
   More complicate mapping could be implemented if necessary
 */
struct prefix_table prefix_tab[] =
{
    {"wl"},
    {"br"},
    {"lan"},
    {"eth"},
    {"usb"},
    {"wsc"},
    {"wps"},
};

#define PREFIX_CNT (sizeof(prefix_tab) /sizeof(struct prefix_table))
#endif

/* Local Function declaration */
static int _nvram_init(void);

/* Debug-related definition */
#define DBG_NVRAM_SET       0x00000001
#define DBG_NVRAM_GET       0x00000002
#define DBG_NVRAM_GETALL    0x00000004
#define DBG_NVRAM_UNSET     0x00000008
#define DBG_NVRAM_COMMIT    0x00000010
#define DBG_NVRAM_UPDATE    0x00000020
#define DBG_NVRAM_INFO      0x00000040
#define DBG_NVRAM_ERROR     0x00000080

#ifdef BCMDBG

static int debug_nvram_level = DBG_NVRAM_ERROR | DBG_NVRAM_GET | DBG_NVRAM_SET;

#define DBG_SET(fmt, arg...) \
        do { if (debug_nvram_level & DBG_NVRAM_SET) \
                printf("%s@%d: "fmt , __FUNCTION__ , __LINE__, ##arg); } while(0)

#define DBG_GET(fmt, arg...) \
        do { if (debug_nvram_level & DBG_NVRAM_GET) \
                printf("%s@%d: "fmt , __FUNCTION__ , __LINE__,##arg); } while(0)

#define DBG_GETALL(fmt, arg...) \
        do { if (debug_nvram_level & DBG_NVRAM_GETALL) \
                printf("%s@%d: "fmt , __FUNCTION__ , __LINE__,##arg); } while(0)

#define DBG_UNSET(fmt, arg...) \
        do { if (debug_nvram_level & DBG_NVRAM_UNSET) \
                printf("%s@%d: "fmt , __FUNCTION__ , __LINE__,##arg); } while(0)

#define DBG_COMMIT(fmt, arg...) \
        do { if (debug_nvram_level & DBG_NVRAM_COMMIT) \
                printf("%s@%d: "fmt , __FUNCTION__ , __LINE__,##arg); } while(0)

#define DBG_UPDATE(fmt, arg...) \
        do { if (debug_nvram_level & DBG_NVRAM_UPDATE) \
                printf("%s@%d: "fmt , __FUNCTION__ , __LINE__, ##arg); } while(0)

#define DBG_INFO(fmt, arg...) \
        do { if (debug_nvram_level & DBG_NVRAM_INFO) \
                printf("%s@%d: "fmt , __FUNCTION__ , __LINE__, ##arg); } while(0)

#define DBG_ERROR(fmt, arg...) \
        do { if (debug_nvram_level & DBG_NVRAM_ERROR) \
                printf("%s@%d: "fmt , __FUNCTION__ , __LINE__, ##arg); } while(0)

#else
#define DBG_SET(fmt, arg...)
#define DBG_GET(fmt, arg...)
#define DBG_GETALL(fmt, arg...)
#define DBG_UNSET(fmt, arg...)
#define DBG_COMMIT(fmt, arg...)
#define DBG_UPDATE(fmt, arg...)
#define DBG_INFO(fmt, arg...)
#define DBG_ERROR(fmt, arg...)
#endif

/*Check nvram variable and return itsmax  value length*/
static unsigned int check_var(char *name)
{
    int idx =0;
    char short_name[64];
#ifdef NVRAM_PREFIX_PARSE
    int cnt = 0;
#endif

    DBG_INFO("Check_var name=[%s]\n", name );
    memset(short_name, 0, sizeof(short_name));

#ifdef NVRAM_PREFIX_PARSE
    /* Remove the Prefix defined in prefix_tab[]. such as, from wl0_ssid to ssid */
    DBG_INFO ( "prefix_tab cnt=%d", PREFIX_CNT );

    strcpy(short_name, name );
    for (cnt=0; cnt<PREFIX_CNT; cnt++) {
        if (!strncmp(name, prefix_tab[cnt].name, strlen(prefix_tab[cnt].name))) {
            /* Skip the chars between prefix_tab[cnt] and "_" */
            for (idx=strlen(prefix_tab[cnt].name); name[idx] !='\0' && name[idx]!='_'; idx++)
                ;
            if (name[idx] == '_')
                strcpy(short_name, name+idx+1);
        }
    }

    DBG_INFO("name=[%s] short_name=[%s]\n", name, short_name );

#else
    if ( !strncmp(name, "wl_", 3) ) {
        strcpy(short_name, name+3);
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else if ( !strncmp(name, "wl0_", 4) ) {
        strcpy( short_name, name+4 );
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else if ( !strncmp(name, "wl1_", 4) ) {
        strcpy( short_name, name+4 );
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else if ( !strncmp(name, "wl0.1_", 6) ) {
        strcpy(short_name, name+6);
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else if ( !strncmp(name, "wl0.2_", 6) ) {
        strcpy(short_name, name+6);
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else if ( !strncmp(name, "wl0.3_", 6) ) {
        strcpy(short_name, name+6);
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else if ( !strncmp(name, "wl1.1_", 6) ) {
        strcpy(short_name, name+6);
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else if ( !strncmp(name, "wl1.2_", 6) ) {
        strcpy(short_name, name+6);
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else if ( !strncmp(name, "wl1.3_", 6) ) {
        strcpy(short_name, name+6);
        DBG_INFO ( "name=[%s] short_name=[%s]\n", name, short_name );
    }
    else {
        strcpy(short_name, name );
    }
#endif

    for ( idx=0; idx < VAR_LEN_COUNT && var_len_tab[idx].name[0] !='\0'; idx++ ) {
        if ( !strcmp( var_len_tab[idx].name, short_name) ) {
            DBG_INFO("[%s] Max Len [%d]\n", name, var_len_tab[idx].max_len );
            return var_len_tab[idx].max_len;
        }
    }
    DBG_INFO("[%s] Default Max Len [%d]\n", name, WL_DEFAULT_VALUE_SIZE_MAX );
    return WL_DEFAULT_VALUE_SIZE_MAX;
}

/* hash Function*/
static INLINE uint hash(const char *s)
{
    uint hash = 0;

    while (*s) {
        hash = 31 * hash + *s++;
    }

    return hash;
}

/*
 * connects by default to 0.0.0.0:5152
 */
static int nvramc_connect_server()
{
    struct sockaddr_in servaddr;
    unsigned int port = NVRAMD_DFLT_PORT;
    char *server_addr = "0.0.0.0";
    int yes = 1;

    memset(&servaddr, 0, sizeof(servaddr));
    if ((nvramc_info.conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DBG_ERROR("socekt failed: %s\n", strerror(errno));
        return -1;
    }

    if(setsockopt(nvramc_info.conn_fd, SOL_TCP, TCP_NODELAY,
        &yes, sizeof(yes)) == -1) {
        DBG_ERROR("setsockopt() error!\n");
        return -1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (isalpha(server_addr[0]) || (inet_pton(AF_INET, \
        (const char *)server_addr, &servaddr.sin_addr) == 0)) {
        DBG_ERROR("Invalid IPADDR: %s\n", server_addr);
    }

    if (connect(nvramc_info.conn_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            printf("connect failed: %s\n", strerror(errno));
            return -1;
    }

    return 0;
}

/* Tuple allocation */
static struct nvram_tuple * _nvram_alloc(struct nvram_tuple *t, const char *name, const char *value)
{
    int len;

    len = check_var( (char *)name );

    if (!(t = _MALLOC_(sizeof(struct nvram_tuple) + strlen(name) + 1 + len + 1))) {
        DBG_ERROR ( "malloc failed\n");
        return NULL;
    }
    memset(&t[1], 0, strlen(name)+1+len+1);
    /* Copy name and value to tuple */
    t->name = (char *) &t[1];
    strcpy(t->name, name);
    t->value = t->name + strlen(name) + 1;

    /* Here: Check value size not larger than sizeof(value) */
    if ( value && strlen(value) > len ) {
        DBG_INFO("%s is too large than allocated size[%d]\n", value, len );
        strncpy(t->value, value, len);
    }
    else
        strcpy(t->value, value?:"");

    return t;
}

static int nvramc_cmd(int resp_need)
{
    int rcount = 0;

    /* Send the command to server */
    if (send(nvramc_info.conn_fd, nvramc_info.cmd_buf, nvramc_info.cmd_size, 0) < 0) {
        DBG_ERROR("Command send failed");
        return -1;
    }

    if (resp_need) {
	fd_set read_fds;

			/* Process the response */
			FD_ZERO(&read_fds);
			FD_SET(nvramc_info.conn_fd, &read_fds);
			if (select(nvramc_info.conn_fd + 1, &read_fds, NULL, NULL, NULL) != -1) {
				if ((rcount = recv(nvramc_info.conn_fd, nvramc_info.cmd_buf, sizeof(nvramc_info.cmd_buf), MSG_DONTWAIT)) < 0) {
					DBG_ERROR("Response receive failed: %s\n", strerror(errno));
					return -1;
				}
			}
			else
				DBG_ERROR("Select failed: %s!\n", strerror(errno));
    } else
        rcount = 1;

    return rcount;
}

/* connect to nvramd server */
static int _nvram_init(void)
{
    if (nvramc_connect_server() < 0)
        return -1;

    nvram_inited=1;

    return 0;
}

/* Global Functions */
int nvram_commit(void)
{
    char *buf = nvramc_info.cmd_buf;
    int rcount = 0;

    DBG_COMMIT("===>nvram_commit\n");

    if(!nvram_inited){
        if (_nvram_init())
            return -1;
    }

    sprintf(nvramc_info.cur_cmd, "commit");
    nvramc_info.cmd_size = \
        sprintf(buf, "%s:", nvramc_info.cur_cmd) + 1;

    rcount = nvramc_cmd(1);
		UNUSED_PARAMETER(rcount);
    DBG_COMMIT("<==nvram_commit\n");

    return 0;
}

char *nvram_get(const char *name)
{
    char *buf = nvramc_info.cmd_buf;
    int rcount = 0;
    uint i;
    struct nvram_tuple *t, *u, **prev;
    char *value;
    int len;
    unsigned long zero = 0;

    DBG_GET("==>nvram_get: %s\n", name);

    if(!nvram_inited){
        if (_nvram_init())
            return NULL;
    }

    sprintf(nvramc_info.cur_cmd, "get");
    nvramc_info.cmd_size = \
        sprintf(buf, "%s:%s", nvramc_info.cur_cmd, name) + 1;

    rcount = nvramc_cmd(1);
    if (rcount > 0)
        nvramc_info.cmd_buf[rcount] = '\0';
    else
        buf = NULL;

    if ((rcount == sizeof(unsigned long)) && memcmp(nvramc_info.cmd_buf, &zero, sizeof(zero)) == 0) {
	DBG_GET("<==nvram_get\n");
	return NULL;
    }
    else {
        i = hash(name) % ARRAYSIZE(nvram_hash);
        /* Find the associated tuple in the hash table */
        for (prev = &nvram_hash[i], t = *prev; t && strcmp(t->name, name); prev = &t->next, t = *prev) {
            ;
        }
        if ( t != NULL && !strcmp(t->name, name) ) {
            /* found the tuple */
            len = check_var( (char *)name );

            /* Here: Check value size not larger than sizeof(value) */
            if ( rcount > len ) {
                DBG_INFO("%s is too large than allocated size[%d]\n", value, len );
                strncpy(t->value, nvramc_info.cmd_buf, len);
            }
            else
                strcpy(t->value, nvramc_info.cmd_buf?:"");
            value = t->value;
        }
        else { /* this is a new tuple */
            if (!(u = _nvram_alloc(t, name, nvramc_info.cmd_buf))) {
                DBG_ERROR("no memory\n");
		DBG_GET("<==nvram_get: no memory!\n");
                return NULL;
            }

            /* Add new tuple to the hash table */
            u->next = nvram_hash[i];
            nvram_hash[i] = u;
            value = u->value;
        }
    }
    DBG_GET("<==nvram_get: %s=%s\n", name, buf);

    return value;
}

/* Nvram variable set. */
int nvram_set(const char *name, const char *value)
{
    char *buf = nvramc_info.cmd_buf;
    int rcount = 0;

    DBG_SET("===>nvram_set[%s]=[%s]\n", name, value?value:"NULL");

    if(!nvram_inited){
        if (_nvram_init())
            return -1;
    }

    sprintf(nvramc_info.cur_cmd, "set");
    if (value)
        nvramc_info.cmd_size = \
            sprintf(buf, "%s:%s=%s", nvramc_info.cur_cmd, name, value) + 1;
    else
        nvramc_info.cmd_size = \
            sprintf(buf, "%s:%s=", nvramc_info.cur_cmd, name) + 1;
    rcount = nvramc_cmd(1);

    DBG_SET("<==nvram_set\n");

    return (rcount < 0) ? rcount : 0;
}

int nvram_unset(const char *name)
{
    char *buf = nvramc_info.cmd_buf;

    DBG_UNSET("===>nvram_unset[%s]\n", name);

    if(!nvram_inited){
        if (_nvram_init())
            return -1;
    }

    sprintf(nvramc_info.cur_cmd, "unset");
    nvramc_info.cmd_size = \
        sprintf(buf, "%s:%s", nvramc_info.cur_cmd, name) + 1;

    nvramc_cmd(0);

    DBG_UNSET("<==nvram_unset\n");

    return 0;
}

char *
nvram_get_bitflag(const char *name, const int bit)
{
	char *ptr;
	unsigned long nvramvalue = 0;
	unsigned long bitflagvalue = 1;

	if (!VALID_BIT(bit))
		return NULL;
	ptr = nvram_get(name);
	if (ptr) {
		bitflagvalue = bitflagvalue << bit;
		nvramvalue = strtoul(ptr, NULL, HEX_BASE);
		if (nvramvalue) {
			nvramvalue = nvramvalue & bitflagvalue;
		}
	}
	return (char *)(ptr ? (nvramvalue ? "1" : "0") : NULL);
}

int
nvram_set_bitflag(const char *name, const int bit, const int value)
{
	char nvram_val[CODE_BUFF];
	char *ptr;
	unsigned long nvramvalue = 0;
	unsigned long bitflagvalue = 1;

	if (!VALID_BIT(bit))
		return 0;
	memset(nvram_val, 0, sizeof(nvram_val));

	ptr = nvram_get(name);
	if (ptr) {
		bitflagvalue = bitflagvalue << bit;
		nvramvalue = strtoul(ptr, NULL, HEX_BASE);
		if (value) {
			nvramvalue |= bitflagvalue;
		} else {
			nvramvalue &= (~bitflagvalue);
		}
	}
	snprintf(nvram_val, sizeof(nvram_val)-1, "%lx", nvramvalue);
	return nvram_set(name, nvram_val);
}

int nvram_getall(char *buf, int count)
{
    int rcount = 0;

    DBG_GET("==>nvram_getall\n");

    if(!nvram_inited){
        if (_nvram_init())
            return -1;
    }

    sprintf(nvramc_info.cur_cmd, "show");
    nvramc_info.cmd_size = \
        sprintf(nvramc_info.cmd_buf, "%s:", nvramc_info.cur_cmd) + 1;

    rcount = nvramc_cmd(1);
    if (rcount > 0) {
        memcpy(buf, nvramc_info.cmd_buf, rcount);
    } else
        buf = NULL;

    DBG_GETALL("<==nvram_getall\n");

    return 0;
}
