
#include <typedefs.h>

#include <bcmsrom_fmt.h>
#include <bcmsrom_tbl.h>
#include "wlu_common.h"
#include "wlu.h"
#include <bcmendian.h>

#include <wps.h>

#include <bcmwifi_rspec.h>

#if defined(linux)
#ifndef TARGETENV_android
#include <unistd.h>
#endif
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#endif /* linux */

#include <802.11ax.h>

#include <miniopt.h>

#include <wlioctl.h>
#include <wlioctl_utils.h>

#if !defined(TARGETOS_nucleus)
#define CLMDOWNLOAD
#endif

#include <epivers.h>
#include <ethernet.h>
#include <802.11.h>
#include <802.1d.h>
#include <802.11e.h>
#include <802.11ax.h>
#include <wpa.h>
#include <bcmip.h>
#include <wps.h>

#include <bcmwifi_rates.h>
#include <bcmwifi_rspec.h>
#include <rte_ioctl.h>

#include <bcmutils.h>
#include <bcmwifi_channels.h>
#include <bcmsrom_fmt.h>
#include <bcmsrom_tbl.h>
#include <typedefs.h>
#include "wlu_common.h"
#include "wlu.h"
#include <bcmcdc.h>
#if defined(linux)
#ifndef TARGETENV_android
#include <unistd.h>
#endif
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <time.h>
#include <sched.h>
#define TIME_STR_SZ 100 /*  string buffer size for timestamp formatting */
#endif /* linux */
#if defined(WLBSSLOAD_REPORT) && defined(linux)
#include <sys/time.h>
#endif   /* defined(WLBSSLOAD_REPORT) && defined(linux) */

#ifdef LINUX
#include <inttypes.h>
#endif
#include <miniopt.h>
#include <errno.h>

#if defined SERDOWNLOAD || defined CLMDOWNLOAD
#include <sys/stat.h>
#include <trxhdr.h>
#ifdef SERDOWNLOAD
#include <usbrdl.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#endif /* SERDOWNLOAD || defined CLMDOWNLOAD */

#include <bcm_mpool_pub.h>
#include <bcmipv6.h>

#include <sdiovar.h>

#include "wlu_subcounters.h"
#include "wluc_otp.h"

static int wl_driver_ioctl(void *wl, wl_ioctl_t *ioc)
{
	struct ifreq *ifr = (struct ifreq *) wl;
	int ret;
	int s;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		syserr("socket");

	/* do it */
	ifr->ifr_data = (caddr_t)ioc;
	ret = ioctl(s, SIOCDEVPRIVATE, ifr);

	/* cleanup */
	close(s);
	return ret;
}

static int (*g_driver_io)(void *wl, wl_ioctl_t *ioc) = wl_driver_ioctl;
int
wl_ioctl(void *wl, int cmd, void *buf, int len, bool set)
{
	wl_ioctl_t ioc;
	int ret;

	/* do it */
	ioc.cmd = cmd;
	ioc.buf = buf;
	ioc.len = len;
	ioc.set = set;

	ret = g_driver_io(wl, &ioc);
	return ret;
}

static int
ioctl_queryinformation_fe(void *wl, int cmd, void* input_buf, unsigned long *input_len)
{
	int error = NO_ERROR;

	error = wl_ioctl(wl, cmd, input_buf, *input_len, FALSE);

	return error;
}

static int
ioctl_setinformation_fe(void *wl, int cmd, void* buf, unsigned long *input_len)
{
	int error = 0;

	error = wl_ioctl(wl,  cmd, buf, *input_len, TRUE);

	return error;
}

int
wl_get(void *wl, int cmd, void *buf, int len)
{
	int error = 0;
	unsigned long input_len = len;

	error = (int)ioctl_queryinformation_fe(wl, cmd, buf, &input_len);

	if (error == BCME_SERIAL_PORT_ERR)
		return BCME_SERIAL_PORT_ERR;
	else if (error == BCME_NODEVICE)
		return BCME_NODEVICE;
	else if (error != 0)
		return BCME_IOCTL_ERROR;

	return 0;
}

int
wl_set(void *wl, int cmd, void *buf, int len)
{
	int error = 0;
	unsigned long input_len = len;

	error = (int)ioctl_setinformation_fe(wl, cmd, buf, &input_len);

	if (error == BCME_SERIAL_PORT_ERR)
		return BCME_SERIAL_PORT_ERR;
	else if (error == BCME_NODEVICE)
		return BCME_NODEVICE;
	else if (error != 0)
		return BCME_IOCTL_ERROR;

	return 0;
}


/*
 *  format an iovar buffer
 *  iovar name is converted to lower case
 */
static uint
wl_iovar_mkbuf(const char *name, char *data, uint datalen, char *iovar_buf, uint buflen, int *perr)
{
	uint iovar_len;
	char *p;

	iovar_len = strlen(name) + 1;

	/* check for overflow */
	if ((iovar_len + datalen) > buflen) {
		*perr = BCME_BUFTOOSHORT;
		return 0;
	}

	/* copy data to the buffer past the end of the iovar name string */
	if (datalen > 0)
		memmove(&iovar_buf[iovar_len], data, datalen);

	/* copy the name to the beginning of the buffer */
	strcpy(iovar_buf, name);

	/* wl command line automatically converts iovar names to lower case for ease of use */
	p = iovar_buf;
	while (*p != '\0') {
		*p = tolower((int)*p);
		p++;
	}

	*perr = 0;
	return (iovar_len + datalen);
}

/* now IOCTL GET commands shall call wlu_get() instead of wl_get() so that the commands
 * can be batched when needed
 */
int
wlu_get(void *wl, int cmd, void *cmdbuf, int len)
{
	if (cmd_batching_mode) {
		if (!WL_SEQ_CMDS_GET_IOCTL_FILTER(cmd)) {
			printf("IOCTL GET command %d is not supported in batching mode\n", cmd);
			return BCME_UNSUPPORTED;
		}			
	}

	return wl_get(wl, cmd, cmdbuf, len);
}

/* now IOCTL SET commands shall call wlu_set() instead of wl_set() so that the commands
 * can be batched when needed
 */
int
wlu_set(void *wl, int cmd, void *cmdbuf, int len)
{
	int err = 0;

	err = wl_set(wl, cmd, cmdbuf, len);
	
	return err;

}

/*
 *  get named iovar providing both parameter and i/o buffers
 *  iovar name is converted to lower case
 */
int
wlu_iovar_getbuf(void* wl, const char *iovar, void *param, int paramlen, void *bufptr, int buflen)
{
	int err;

	wl_iovar_mkbuf(iovar, param, paramlen, bufptr, buflen, &err);
	if (err)
		return err;

	return wlu_get(wl, WLC_GET_VAR, bufptr, buflen);
}

/*
 *  set named iovar providing both parameter and i/o buffers
 *  iovar name is converted to lower case
 */
int
wlu_iovar_setbuf(void* wl, const char *iovar, void *param, int paramlen, void *bufptr, int buflen)
{
	int err;
	int iolen;

	iolen = wl_iovar_mkbuf(iovar, param, paramlen, bufptr, buflen, &err);
	if (err)
		return err;

	return wlu_set(wl, WLC_SET_VAR, bufptr, iolen);
}

/*
 *  get named iovar without parameters into a given buffer
 *  iovar name is converted to lower case
 */
int
wlu_iovar_get(void *wl, const char *iovar, void *outbuf, int len)
{
	char smbuf[WLC_IOCTL_SMLEN];
	int err;

	/* use the return buffer if it is bigger than what we have on the stack */
	if (len > (int)sizeof(smbuf)) {
		err = wlu_iovar_getbuf(wl, iovar, NULL, 0, outbuf, len);
	} else {
		memset(smbuf, 0, sizeof(smbuf));
		err = wlu_iovar_getbuf(wl, iovar, NULL, 0, smbuf, sizeof(smbuf));
		if (err == 0)
			memcpy(outbuf, smbuf, len);
	}

	return err;
}

/*
 *  set named iovar given the parameter buffer
 *  iovar name is converted to lower case
 */
int
wlu_iovar_set(void *wl, const char *iovar, void *param, int paramlen)
{
	char smbuf[WLC_IOCTL_SMLEN*2];

	memset(smbuf, 0, sizeof(smbuf));

	return wlu_iovar_setbuf(wl, iovar, param, paramlen, smbuf, sizeof(smbuf));
}

/*
 *  get named iovar as an integer value
 *  iovar name is converted to lower case
 */
int
wlu_iovar_getint(void *wl, const char *iovar, int *pval)
{
	int ret;

	ret = wlu_iovar_get(wl, iovar, pval, sizeof(int));
	if (ret >= 0)
	{
		*pval = dtoh32(*pval);
	}
	return ret;
}

/*
 *  set named iovar given an integer parameter
 *  iovar name is converted to lower case
 */
int
wlu_iovar_setint(void *wl, const char *iovar, int val)
{
	val = htod32(val);
	return wlu_iovar_set(wl, iovar, &val, sizeof(int));
}

int
wlu_var_getbuf(void *wl, const char *iovar, void *param, int param_len, void **bufptr)
{
	int len;

	memset(buf, 0, WLC_IOCTL_MAXLEN);
	strcpy(buf, iovar);

	/* include the null */
	len = strlen(iovar) + 1;

	if (param_len)
		memcpy(&buf[len], param, param_len);

	*bufptr = buf;

	return wlu_get(wl, WLC_GET_VAR, &buf[0], WLC_IOCTL_MAXLEN);
}

int
wlu_var_setbuf(void *wl, const char *iovar, void *param, int param_len)
{
	int len;

	memset(buf, 0, WLC_IOCTL_MAXLEN);
	strcpy(buf, iovar);

	/* include the null */
	len = strlen(iovar) + 1;

	if (param_len)
		memcpy(&buf[len], param, param_len);

	len += param_len;

	return wlu_set(wl, WLC_SET_VAR, &buf[0], len);
}

/* wl sta_info */
/*****************************************************************
Parameters:	wl: 	struct ifreq ifr;
			strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
			ifname: wl0 or wl1
		ea:	sta mac address
		buf:	return sta info buf, need alloc mem
Return:		0 on success, -1 on failure
Description:	wl sta_info, refer to wl_sta_info
*****************************************************************/
int
bcm_get_sta_info(void *wl, char *ea, char *buf)
{
	sta_info_t *sta;
	char *param;
	int err;

	memset(buf, 0, WLC_IOCTL_MAXLEN);
	strcpy(buf, "sta_info");
	buflen = strlen(buf) + 1;
	param = (char *)(buf + buflen);
	memcpy(param, ea, ETHER_ADDR_LEN);

	if ((err = wlu_get(wl, WLC_GET_VAR, buf, WLC_IOCTL_MAXLEN)) < 0) {
		return err;	
	}
	 
	/* BRCM print example */
	//err = wl_sta_info_print(wl, buf);
	
	return 0;
}


int
wl_sta_info_print(void *wl, void *buf)
{
	sta_info_v4_t *sta;
	sta_info_v5_t *sta_v5;
	sta_info_v7_t *sta_v7 = NULL;
	sta_info_v8_t *sta_v8 = NULL;
	int i, err = BCME_OK;
	char buf_chanspec[20];
	uint32 rxdur_total = 0;
	bool have_rxdurtotal = FALSE;
	chanspec_t chanspec;
	bool have_chanspec = FALSE;
	wl_rateset_args_u_t *rateset_adv;
	bool have_rateset_adv = FALSE;
	uint32 wpauth = 0;
	uint8 algo = 0;
	int8 srssi = 0;
	uint8 rrm_cap[DOT11_RRM_CAP_LEN];

	/* display the sta info */
	sta = (sta_info_v4_t *)buf;
	sta->ver = dtoh16(sta->ver);
	sta->len = dtoh16(sta->len);

	/* Report unrecognized version */
	if (sta->ver < WL_STA_VER_4) {
		printf(" ERROR: unsupported driver station info version %d\n", sta->ver);
		return BCME_ERROR;
	}
	else if (sta->ver == WL_STA_VER_4) {
		rxdur_total = dtoh32(sta->rx_dur_total);
		have_rxdurtotal = TRUE;
		if (sta->len >= STRUCT_SIZE_THROUGH(sta, rateset_adv)) {
			chanspec = dtoh16(sta->chanspec);
			wf_chspec_ntoa(chanspec, buf_chanspec);
			have_chanspec = TRUE;

			rateset_adv = (wl_rateset_args_u_t *)&sta->rateset_adv;
			have_rateset_adv = TRUE;
		}
	}
	else if (sta->ver == WL_STA_VER_5) {
		sta_v5 = (sta_info_v5_t *)buf;
		chanspec = dtoh16(sta_v5->chanspec);
		wf_chspec_ntoa(chanspec, buf_chanspec);
		have_chanspec = TRUE;

		rateset_adv = (wl_rateset_args_u_t *)&sta_v5->rateset_adv;
		have_rateset_adv = TRUE;
	}
	else if (sta->ver >= WL_STA_VER_7) {
		sta_v7 = (sta_info_v7_t *)buf;

		rxdur_total = dtoh32(sta_v7->rx_dur_total);
		have_rxdurtotal = TRUE;

		chanspec = dtoh16(sta_v7->chanspec);
		wf_chspec_ntoa(chanspec, buf_chanspec);
		have_chanspec = TRUE;

		if (sta->ver == WL_STA_VER_8) {
			sta_v8 = (sta_info_v8_t *)buf;
			wpauth = dtoh32(sta_v8->wpauth);
			srssi = dtoh32(sta_v8->srssi);
			sta_v8->he_flags = dtoh32(sta_v8->he_flags);
			sta_v8->he_omi = dtoh16(sta_v8->he_omi);
			memcpy(rrm_cap, sta_v8->rrm_capabilities, DOT11_RRM_CAP_LEN);
		}
		else if (sta->ver == WL_STA_VER_7) {
			sta_v7->he_flags = dtoh16(sta_v7->he_flags);
			wpauth = dtoh16(sta_v7->wpauth);
		}
		algo = sta_v7->algo;

		rateset_adv = (wl_rateset_args_u_t *)&sta_v7->rateset_adv;
		have_rateset_adv = TRUE;
	}
	else {
		printf(" ERROR: unknown driver station info version %d\n", sta->ver);
		return BCME_ERROR;
	}

	sta->cap = dtoh16(sta->cap);
	sta->aid = dtoh16(sta->aid);
	sta->flags = dtoh32(sta->flags);
	sta->idle = dtoh32(sta->idle);
	sta->rateset.count = dtoh32(sta->rateset.count);
	sta->in = dtoh32(sta->in);
	sta->listen_interval_inms = dtoh32(sta->listen_interval_inms);
	sta->ht_capabilities = dtoh16(sta->ht_capabilities);
	sta->vht_flags = dtoh16(sta->vht_flags);

	printf("[VER %d] STA %s:\n", sta->ver, wl_ether_etoa(&sta->ea));
	if (have_chanspec) {
		printf("\t chanspec %s (0x%x)\n", buf_chanspec, chanspec);
	}
	printf("\t aid:%d ", WL_STA_AID(sta->aid));
	printf("\n\t rateset ");
	dump_rateset(sta->rateset.rates, sta->rateset.count);
	printf("\n\t idle %d seconds\n", sta->idle);
	printf("\t in network %d seconds\n", sta->in);
	printf("\t state:%s%s%s\n",
	       (sta->flags & WL_STA_AUTHE) ? " AUTHENTICATED" : "",
	       (sta->flags & WL_STA_ASSOC) ? " ASSOCIATED" : "",
	       (sta->flags & WL_STA_AUTHO) ? " AUTHORIZED" : "");

	if (sta->ver >= WL_STA_VER_7) {

		printf("\t connection:%s\n",
				(wpauth > 0x01) ? " SECURED" : "OPEN");

		if (wpauth == 0x00)
			printf("\t auth: %s",  "AUTH-DISABLED");	/* Legacy (i.e., non-WPA) */
		else if (wpauth == 0x1)
			printf("\t auth: %s",  "AUTH-NONE");		/* none (IBSS) */
		else if (wpauth == 0x2)
			printf("\t auth: %s",  "AUTH-UNSPECIFIED");	/* over 802.1x */
		else if (wpauth == 0x4)
			printf("\t auth: %s",  "WPA-PSK");		/* Pre-shared key */
		else if (wpauth == 0x40)
			printf("\t auth: %s",  "WPA-PSK");		/* over 802.1x */
		else if (wpauth == 0x80)
			printf("\t auth: %s",  "WPA2-PSK");		/* Pre-shared key */
		else if (wpauth == 0x84)
			printf("\t auth: %s",  "WPA-PSK + WPA2-PSK");	/* Pre-shared key */
		else if (wpauth == 0x100)
			printf("\t auth: %s",  "BRCM_AUTH_PSK");	/* BRCM specific PSK */
		else if (wpauth == 0x200)
			printf("\t auth: %s",  "BRCM_AUTH_DPT");  /* DPT PSK without group keys */
		else if (wpauth == 0x1000)
			printf("\t auth: %s",  "WPA2_AUTH_MFP");	/* MFP (11w) */
		else if (wpauth == 0x2000)
			printf("\t auth: %s",  "WPA2_AUTH_TPK");	/* TDLS Peer Key */
		else if (wpauth == 0x4000)
			printf("\t auth: %s",  "WPA2_AUTH_FT");		/* Fast Transition */
		else if (wpauth == 0x4080)
			printf("\t auth: %s",  "WPA2-PSK+FT");		/* Fast Transition */
		else if (wpauth == 0x4084)
			printf("\t auth: %s",  "WPA-PSK + WPA2-PSK + FT");  /* Fast Transition */
		else if (wpauth == 0x40000)
			printf("\t auth: %s",  "WPA3-SAE");		/* WPA3-SAE */
		else if (wpauth == 0x40080)
			printf("\t auth: %s",  "WPA2-PSK WPA3-SAE");	/* WPA2-PSK + WPA3-SAE */
		else
			printf("\t auth: %s",  "UNKNOWN AUTH");		/* Unidentified */
		printf("\n\t crypto: %s\n",   bcm_crypto_algo_name(algo));
	}

	printf("\t flags 0x%x:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	       sta->flags,
	       (sta->flags & WL_STA_BRCM) ? " BRCM" : "",
	       (sta->flags & WL_STA_WME) ? " WME" : "",
	       (sta->flags & WL_STA_PS) ? " PS" : "",
	       (sta->flags & WL_STA_NONERP) ? " No-ERP" : "",
	       (sta->flags & WL_STA_APSD_BE) ? " APSD_BE" : "",
	       (sta->flags & WL_STA_APSD_BK) ? " APSD_BK" : "",
	       (sta->flags & WL_STA_APSD_VI) ? " APSD_VI" : "",
	       (sta->flags & WL_STA_APSD_VO) ? " APSD_VO" : "",
	       (sta->flags & WL_STA_N_CAP) ? " N_CAP" : "",
	       (sta->flags & WL_STA_VHT_CAP) ? " VHT_CAP" : "",
	       (sta->flags & WL_STA_HE_CAP) ? " HE_CAP" : "",
	       (sta->flags & WL_STA_AMPDU_CAP) ? " AMPDU" : "",
	       (sta->flags & WL_STA_AMSDU_CAP) ? " AMSDU" : "",
	       (sta->flags & WL_STA_MIMO_PS) ? " MIMO-PS" : "",
	       (sta->flags & WL_STA_MIMO_RTS) ? " MIMO-PS-RTS" : "",
	       (sta->flags & WL_STA_RIFS_CAP) ? " RIFS" : "",
	       (sta->flags & WL_STA_WPS) ? " WPS" : "",
	       (sta->flags & WL_STA_GBL_RCLASS) ? " GBL_RCLASS" : "",
	       (sta->flags & WL_STA_DWDS_CAP) ? " DWDS_CAP": "",
	       (sta->flags & WL_STA_DWDS) ? " DWDS_ACTIVE" : "",
	       (sta->flags & WL_STA_WDS) ? " WDS" : "",
	       (sta->flags & WL_STA_WDS_LINKUP) ? " WDS_LINKUP" : "");

	printf("\t HT caps 0x%x:%s%s%s%s%s%s%s%s%s\n",
		sta->ht_capabilities,
	       (sta->ht_capabilities & WL_STA_CAP_LDPC_CODING) ? " LDPC" : "",
	       (sta->ht_capabilities & WL_STA_CAP_40MHZ) ? " 40MHz" : " ",
	       (sta->ht_capabilities & WL_STA_CAP_GF) ? " GF" : "",
	       (sta->ht_capabilities & WL_STA_CAP_SHORT_GI_20) ? " SGI20" : "",
	       (sta->ht_capabilities & WL_STA_CAP_SHORT_GI_40) ? " SGI40" : "",
	       (sta->ht_capabilities & WL_STA_CAP_TX_STBC) ? " STBC-Tx" : "",
	       (sta->ht_capabilities & WL_STA_CAP_RX_STBC_MASK) ? " STBC-Rx" : "",
	       (sta->ht_capabilities & WL_STA_CAP_DELAYED_BA) ? " D-BlockAck" : "",
	       (sta->ht_capabilities & WL_STA_CAP_40MHZ_INTOLERANT) ? " 40-Intl" : "");

	if (sta->flags & WL_STA_VHT_CAP) {
		printf("\t VHT caps 0x%x:%s%s%s%s%s%s%s%s%s%s%s\n",
			sta->vht_flags,
			(sta->vht_flags & WL_STA_VHT_LDPCCAP) ? " LDPC" : "",
			(sta->vht_flags & WL_STA_SGI80) ? " SGI80" : "",
			(sta->vht_flags & WL_STA_SGI160) ? " SGI160" : "",
			(sta->vht_flags & WL_STA_VHT_TX_STBCCAP) ? " STBC-Tx" : "",
			(sta->vht_flags & WL_STA_VHT_RX_STBCCAP) ? " STBC-Rx" : "",
			(sta->vht_flags & WL_STA_SU_BEAMFORMER) ? " SU-BFR" : "",
			(sta->vht_flags & WL_STA_SU_BEAMFORMEE) ? " SU-BFE" : "",
			(sta->vht_flags & WL_STA_MU_BEAMFORMER) ? " MU-BFR" : "",
			(sta->vht_flags & WL_STA_MU_BEAMFORMEE) ? " MU-BFE" : "",
			(sta->vht_flags & WL_STA_VHT_TXOP_PS) ? " TXOPPS" : "",
			(sta->vht_flags & WL_STA_HTC_VHT_CAP) ? " VHT-HTC" : "");
	}

	if (sta_v7 && (sta_v7->flags & WL_STA_HE_CAP)) {
		printf("\t HE caps 0x%x:%s%s%s%s%s%s%s\n",
			sta_v7->he_flags,
			(sta_v7->he_flags & WL_STA_HE_LDPCCAP) ? " LDPC" : "",
			(sta_v7->he_flags & WL_STA_HE_TX_STBCCAP) ? " STBC-Tx" : "",
			(sta_v7->he_flags & WL_STA_HE_RX_STBCCAP) ? " STBC-Rx" : "",
			(sta_v7->he_flags & WL_STA_HE_HTC_CAP) ? " HE-HTC" : "",
			(sta_v7->he_flags & WL_STA_HE_SU_BEAMFORMER) ? " SU-BFR" : "",
			(sta_v7->he_flags & WL_STA_HE_SU_MU_BEAMFORMEE) ? " SU&MU-BFE" : "",
			(sta_v7->he_flags & WL_STA_HE_MU_BEAMFORMER) ? " MU-BFR" : "");
	}

	if (sta_v8 && (sta_v8->flags & WL_STA_HE_CAP)) {
		const char *_bw[] = {"20", "40", "80", "160"};
		printf("\t OMI 0x%04x: ", sta_v8->he_omi);
		printf("%sMhz ", _bw[HTC_OM_CONTROL_CHANNEL_WIDTH(sta_v8->he_omi)]);
		printf("rx=%dss ", 1 + HTC_OM_CONTROL_RX_NSS(sta_v8->he_omi));
		printf("tx=%dss ", 1 + HTC_OM_CONTROL_TX_NSTS(sta_v8->he_omi));
		if (HTC_OM_CONTROL_ER_SU_DISABLE(sta_v8->he_omi))
			printf("ER_SU_DISABLE ");
		if (HTC_OM_CONTROL_UL_MU_DISABLE(sta_v8->he_omi))
			printf("UL_MU_DISABLE ");
		if (HTC_OM_CONTROL_UL_MU_DATA_DISABLE(sta_v8->he_omi))
			printf("UL_MU_DATA_DISABLE ");
		if (HTC_OM_CONTROL_DL_MUMIMO_RESOUND(sta_v8->he_omi))
			printf("DL_MUMIMO_RESOUND ");
		printf("\n");
		if (sta_v8->twt_info != 0) {
			printf("\t TWT info 0x%x:%s%s%s\n",
				sta_v8->twt_info,
				(sta_v8->twt_info & WL_STA_TWT_CAP) ? " CAPABLE" : "",
				(sta_v8->twt_info & WL_STA_TWT_SCHEDULED) ? " NEGOTIATED" : "",
				(sta_v8->twt_info & WL_STA_TWT_ACTIVE) ? " ACTIVE" : (
					(sta_v8->twt_info & WL_STA_TWT_SCHEDULED) ? " PAUSED": ""));
		}
	}
	if (sta->flags & WL_STA_SCBSTATS)
	{
		printf("\t tx total pkts: %d\n", dtoh32(sta->tx_tot_pkts));
		printf("\t tx total bytes: %llu\n", dtoh64(sta->tx_tot_bytes));
		printf("\t tx ucast pkts: %d\n", dtoh32(sta->tx_pkts));
		printf("\t tx ucast bytes: %llu\n", dtoh64(sta->tx_ucast_bytes));
		printf("\t tx mcast/bcast pkts: %d\n", dtoh32(sta->tx_mcast_pkts));
		printf("\t tx mcast/bcast bytes: %llu\n", dtoh64(sta->tx_mcast_bytes));
		printf("\t tx failures: %d\n", dtoh32(sta->tx_failures));
		printf("\t rx data pkts: %d\n", dtoh32(sta->rx_tot_pkts));
		printf("\t rx data bytes: %llu\n", dtoh64(sta->rx_tot_bytes));
		if (have_rxdurtotal) {
			printf("\t rx data dur: %u\n", rxdur_total);
		}
		printf("\t rx ucast pkts: %d\n", dtoh32(sta->rx_ucast_pkts));
		printf("\t rx ucast bytes: %llu\n", dtoh64(sta->rx_ucast_bytes));
		printf("\t rx mcast/bcast pkts: %d\n", dtoh32(sta->rx_mcast_pkts));
		printf("\t rx mcast/bcast bytes: %llu\n", dtoh64(sta->rx_mcast_bytes));
		printf("\t rate of last tx pkt: %d kbps - %d kbps\n",
			dtoh32(sta->tx_rate), dtoh32(sta->tx_rate_fallback));
		printf("\t rate of last rx pkt: %d kbps\n", dtoh32(sta->rx_rate));
		printf("\t rx decrypt succeeds: %d\n", dtoh32(sta->rx_decrypt_succeeds));
		printf("\t rx decrypt failures: %d\n", dtoh32(sta->rx_decrypt_failures));
		printf("\t tx data pkts retried: %d\n", dtoh32(sta->tx_pkts_retried));

		for (i = WL_ANT_IDX_1; i < WL_RSSI_ANT_MAX; i++) {
			if (i == WL_ANT_IDX_1)
				printf("\t per antenna rssi of last rx data frame:");
			printf(" %d", dtoh32(sta->rx_lastpkt_rssi[i]));
			if (i == WL_RSSI_ANT_MAX-1)
				printf("\n");
		}
		for (i = WL_ANT_IDX_1; i < WL_RSSI_ANT_MAX; i++) {
			if (i == WL_ANT_IDX_1)
				printf("\t per antenna average rssi of rx data frames:");
			printf(" %d", dtoh32(sta->rssi[i]));
			if (i == WL_RSSI_ANT_MAX-1)
				printf("\n");
		}
		for (i = WL_ANT_IDX_1; i < WL_RSSI_ANT_MAX; i++) {
			if (i == WL_ANT_IDX_1)
				printf("\t per antenna noise floor:");
			printf(" %d", dtoh32(sta->nf[i]));
			if (i == WL_RSSI_ANT_MAX-1)
				printf("\n");
		}

		printf("\t tx total pkts sent: %d\n", dtoh32(sta->tx_pkts_total));
		printf("\t tx pkts retries: %d\n", dtoh32(sta->tx_pkts_retries));
		printf("\t tx pkts retry exhausted: %d\n", dtoh32(sta->tx_pkts_retry_exhausted));
		printf("\t tx FW total pkts sent: %d\n", dtoh32(sta->tx_pkts_fw_total));
		printf("\t tx FW pkts retries: %d\n", dtoh32(sta->tx_pkts_fw_retries));
		printf("\t tx FW pkts retry exhausted: %d\n",
			dtoh32(sta->tx_pkts_fw_retry_exhausted));
		printf("\t rx total pkts retried: %d\n", dtoh32(sta->rx_pkts_retried));
	}
	/* Driver didn't return extended station info */
	if (sta->len < sizeof(sta_info_v5_t)) {
		return 0;
	}

	if (have_rateset_adv) {
		int rslen = 0, rsver = 0;
		uint8 *rs_mcs = NULL;
		uint16 *rs_vht_mcs = NULL;
		uint16 *rs_he_mcs = NULL;
		if (sta->ver >= WL_STA_VER_7) {
			rs_mcs = rateset_adv->rsv2.mcs;
			rs_vht_mcs = rateset_adv->rsv2.vht_mcs;
			rs_he_mcs = rateset_adv->rsv2.he_mcs;
		} else {
			if ((err = wl_get_rateset_args_info(wl, &rslen, &rsver)) < 0)
				return (err);
			wl_rateset_get_fields(rateset_adv, rsver, NULL, NULL, &rs_mcs,
				&rs_vht_mcs, NULL);
		}
		wl_print_mcsset((char *)rs_mcs);
		wl_print_vhtmcsset((uint16 *)rs_vht_mcs);
		if (rs_he_mcs != NULL && rs_he_mcs[0] != 0xffff) {
			printf("HE SET  :\n");
			wl_print_hemcsnss((uint16 *)rs_he_mcs);
		}
	}

	if (sta->ver >= WL_STA_VER_7)
	{
		printf("smoothed rssi: %d\n", srssi);
		printf("tx nrate\n");
		wl_nrate_print(dtoh32(sta_v7->tx_rspec), WLC_IOCTL_VERSION);
		printf("rx nrate\n");
		wl_nrate_print(dtoh32(sta_v7->rx_rspec), WLC_IOCTL_VERSION);
		printf("wnm\n");
		wl_wnm_print(sta_v7->wnm_cap);
		if (sta_v7->len >= STRUCT_SIZE_THROUGH(sta_v7, sta_vendor_oui)) {
			for (i = 0; i < sta_v7->sta_vendor_oui.count && i < WLC_MAX_ASSOC_OUI_NUM;
					i ++) {
				printf("VENDOR OUI VALUE[%d] %02X:%02X:%02X \n", i,
						sta_v7->sta_vendor_oui.oui[i][0],
						sta_v7->sta_vendor_oui.oui[i][1],
						sta_v7->sta_vendor_oui.oui[i][2]);
			}
		}
		if (sta_v7->len >= STRUCT_SIZE_THROUGH(sta_v7, link_bw)) {
			switch (sta_v7->link_bw) {
				case 1:
					printf("link bandwidth = 20 MHZ \n");
					break;
				case 2:
					printf("link bandwidth = 40 MHZ \n");
					break;
				case 3:
					printf("link bandwidth = 80 MHZ \n");
					break;
				case 4:
					printf("link bandwidth = 160 MHZ \n");
					break;
			}
		}
		wl_rrm_print(rrm_cap);
	}

	return (0);
}


/* wl bs_data */
static void
wl_scb_bs_data_convert_v2(iov_bs_data_struct_t *v2)
{
	/* This only take care of endianess between driver and application */
	int argn;
	for (argn = 0; argn < v2->structure_count; ++argn) {
		iov_bs_data_record_t *rec;
		iov_bs_data_counters_t *ctr;

		rec = &v2->structure_record[argn];
		ctr = &rec->station_counters;

		rec->station_flags = dtoh16(rec->station_flags);

#define DEVICE_TO_HOST64(xyzzy) ctr->xyzzy = dtoh64(ctr->xyzzy)
#define DEVICE_TO_HOST32(xyzzy) ctr->xyzzy = dtoh32(ctr->xyzzy)
		DEVICE_TO_HOST64(throughput);
		DEVICE_TO_HOST64(txrate_main);
		DEVICE_TO_HOST64(txrate_succ);
		DEVICE_TO_HOST32(retry_drop);
		DEVICE_TO_HOST32(rtsfail);
		DEVICE_TO_HOST32(retry);
		DEVICE_TO_HOST32(acked);
		DEVICE_TO_HOST32(ru_acked);
		DEVICE_TO_HOST32(mu_acked);
		DEVICE_TO_HOST32(time_delta);
		DEVICE_TO_HOST32(airtime);
		DEVICE_TO_HOST32(txbw);
		DEVICE_TO_HOST32(txnss);
		DEVICE_TO_HOST32(txmcs);
#undef DEVICE_TO_HOST64
#undef DEVICE_TO_HOST32
	}
}

static void
wl_scb_bs_data_convert_v1(iov_bs_data_struct_v1_t *v1, iov_bs_data_struct_t *v2)
{
	int argn;
	int max_stations;

	v2->structure_version = v1->structure_version;
	v2->structure_count = v1->structure_count;

	/* Calculating the maximum number of stations that v2 can hold  */
	max_stations = (WLC_IOCTL_MAXLEN / sizeof(iov_bs_data_struct_t)) - 1;

	for (argn = 0; (argn < v1->structure_count) &&
		(argn < max_stations); ++argn) {
		iov_bs_data_record_v1_t *rec_v1;
		iov_bs_data_counters_v1_t *ctr_v1;
		iov_bs_data_record_t *rec_v2;
		iov_bs_data_counters_t *ctr_v2;

		rec_v2 = &v2->structure_record[argn];
		ctr_v2 = &rec_v2->station_counters;

		rec_v1 = &v1->structure_record[argn];
		ctr_v1 = &rec_v1->station_counters;

		memcpy(&rec_v2->station_address, &rec_v1->station_address, ETHER_ADDR_LEN);
		rec_v2->station_flags = dtoh16(rec_v1->station_flags);

		ctr_v2->throughput = (uint64)dtoh32(ctr_v1->throughput);
		ctr_v2->txrate_main = (uint64)dtoh32(ctr_v1->txrate_main);
		ctr_v2->txrate_succ = (uint64)dtoh32(ctr_v1->txrate_succ);
		ctr_v2->txrate_succ *= (PERF_LOG_RATE_FACTOR_500 / PERF_LOG_RATE_FACTOR_100);

		ctr_v2->retry_drop = dtoh32(ctr_v1->retry_drop);
		ctr_v2->rtsfail = dtoh32(ctr_v1->rtsfail);
		ctr_v2->retry = dtoh32(ctr_v1->retry);
		ctr_v2->acked = dtoh32(ctr_v1->acked);
		ctr_v2->time_delta = dtoh32(ctr_v1->time_delta);
		ctr_v2->airtime = dtoh32(ctr_v1->airtime);

		ctr_v2->txbw = 0;
		ctr_v2->txmcs = 0;
		ctr_v2->txnss = 0;
		ctr_v2->ru_acked = 0;
		ctr_v2->mu_acked = 0;
	}
}

/*****************************************************************
 * Parameters:    wl:		struct ifreq ifr;
 * 				strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
 *				ifname: wl0 or wl1
 * 		  buf:    return bs_data buf, need alloc mem
 * Return:         0 on success, -1 on failure
 * Description:    wl sta_info, refer to wl_scb_bs_data
 ******************************************************************/
int
bcm_get_bs_data(void *wl, char *buf)
{
	int err;
	int32 flag_bits = 0;
	iov_bs_data_struct_t *data = (iov_bs_data_struct_t *)buf;
	void *p = NULL;
	
	err = wlu_iovar_getbuf(wl, "bs_data", &flag_bits, sizeof(flag_bits), buf, WLC_IOCTL_MAXLEN);
	if (err) {
		return (err);
	}
	data->structure_version = dtoh16(data->structure_version);
	data->structure_count = dtoh16(data->structure_count);
	if (data->structure_count == 0) {
		printf("No stations are currently associated.\n");
		return BCME_OK;
	}

	if (data->structure_version == SCB_BS_DATA_STRUCT_VERSION_v1) {
		/* Alloc some memory and convert all incoming v1 format
		 * into v2, and redirect data to it
		 */
		p = malloc(WLC_IOCTL_MAXLEN);
		if (!p) {
			return BCME_NOMEM;
		}
		wl_scb_bs_data_convert_v1((iov_bs_data_struct_v1_t *)data,
			(iov_bs_data_struct_t *)p);
		data = (iov_bs_data_struct_t*)p;
		v1_style = TRUE;
	} else if (data->structure_version == SCB_BS_DATA_STRUCT_VERSION) {
		wl_scb_bs_data_convert_v2(data);
	} else {
		return BCME_IOCTL_ERROR;
	}
	
		/* Sum up total throughput */
	for (argn = 0; argn < data->structure_count; ++argn) {
		iov_bs_data_record_t *rec;
		iov_bs_data_counters_t *ctr;
		float data_rate;

		rec = &data->structure_record[argn];
		ctr = &rec->station_counters;

		/* Save delta_time for total_airtime calculation */
		if (ctr->time_delta)
			time_delta = ctr->time_delta;

		/* Calculate data rate in bits per second, rather than bytes per second */
		data_rate = (ctr->time_delta) ?
			(float)((float)ctr->throughput * 8.0 / (float)ctr->time_delta) : 0.0;

		total_throughput += data_rate;
		cumul_airtime += ctr->airtime;
	}
	return 0;
}

/* wl chcanim_stats */

/*****************************************************************
 * Parameters:     wl:     struct ifreq ifr;
 *                         strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
 *                         ifname: wl0 or wl1
 * 		   buf:    return chanim_stats buf, need not alloc mem, we will alloc mem in wlu_var_getbuf
 * Return:         0 on success, -1 on failure
 * Description:    wl chanim_stats, refer to wl_chanim_stats
 ******************************************************************/
int
bcm_get_chanim_stats(void *wl, char *buf)
{
	wl_chanim_stats_t param;
	wl_chanim_stats_t *stats;
	int stats_size;
	int  err;
	void *ptr;
	
	param.buflen = htod32(sizeof(wl_chanim_stats_t));
	param.count = htod32(WL_CHANIM_COUNT_ONE);
	
	stats_size = WL_CHANIM_STATS_FIXED_LEN +
		MAX(sizeof(chanim_stats_t), sizeof(chanim_stats_v2_t));
	stats = (wl_chanim_stats_t *)malloc(stats_size);
	if (stats == NULL) {
		fprintf(stderr, "memory alloc failure\n");
		return BCME_NOMEM;
	}
	memset(stats, 0, stats_size);
	stats->buflen = htod32(stats_size);
	if ((err = wlu_var_getbuf(wl, "chanim_stats", stats, stats_size, &ptr)) < 0) {
		printf("failed to get chanim results");
		free(stats);
		return err;
	}
	memcpy(stats, ptr, stats_size);
	stats->version = dtoh32(stats->version);
	if (!((stats->version == WL_CHANIM_STATS_VERSION) ||
		(stats->version == WL_CHANIM_STATS_VERSION_2))) {
		printf("Sorry, your driver has wl_chanim_stats version %d "
			"but this program supports only version %d and %d.\n",
			stats->version, WL_CHANIM_STATS_VERSION_2, WL_CHANIM_STATS_VERSION);
		free(stats);
		return -1;
	}

	/* get fw chanim stats */
	if ((err = wlu_var_getbuf(wl, "chanim_stats", &param, stats_size, &buf)) < 0) {
		printf("failed to get chanim results");
		free(stats);
		return err;
	}

	/*BRCM chanim_stats print function */
	//wl_chanim_stats_print(buf, param.count);

	free(stats);
	return (err);

}


/* wl dlystats/dlystats_clear */

#define CALLOC(n_bytes) calloc(n_bytes, sizeof(uint8)) /* allows start address to be byte aligned */

/*****************************************************************
Parameters:	wl: 	struct ifreq ifr;
			strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
			ifname: wl0 or wl1
		ea:	sta mac address
		buf:	return sta dlystats info, need alloc mem
Return:		0 on success, -1 on failure
Description:	wl dlystats/dlystats_clear, refer to wl_dlystats and wl_dlystats_clear
*****************************************************************/
int bcm_get_dlystats(void *wl, char *ea, char *buf)
{
	int ret = 0;
	txdelay_stats_t *dlystats_param;
	txdelay_stats_t *delay_stats;

	dlystats_param = (txdelay_stats_t *)CALLOC(sizeof(*dlystats_param));

	if (dlystats_param == NULL) {
		return BCME_NOMEM;
	}

	dlystats_param->version = TXDELAY_STATS_VERSION;
	memcpy(&dlystats_param->scb_delay_stats[0].ea, ea, ETHER_ADDR_LEN);
	dlystats_param->scb_cnt = 1;
	
	ret = wlu_iovar_getbuf(wl, "dlystats", dlystats_param, sizeof(*dlystats_param),
			buf, WLC_IOCTL_MAXLEN);
	
	if (ret) {
		if (ret == BCME_VERSION) {
			printf("txdelay_stats_t version is mismatched\n");
		}
		goto EXIT;
	}

	/* BRCM dlystats print function */
	//delay_stats = (txdelay_stats_t *)buf;
	//dlystat_dump(delay_stats);

EXIT:
	free(dlystats_param);
	return ret;
}

int
bcm_dlystats_clear(void *wl)
{
	int err = 0;
	char bufdata[WLC_IOCTL_MAXLEN];
	
	err = wlu_iovar_set(wl, "dlystats_clear", bufdata, 0);
	
	return err;
	
}
	
/* pktq_stats */

/*****************************************************************
 * Parameters:     wl:    		struct ifreq ifr;
 * 			   		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
 * 			   		ifname: wl0 or wl1
 * 		   input_params:	num_addr: number of sta, max shall be 4, here only think about 1 sta
 * 		   			addr_type: pktq_stats mode, 'C'=common queue, 'A'= ampdu queue
 * 		   			ea: sta mac addr
 * 		   buf:    		return pktq_stats buf, need alloc mem
 * Return:         0 on success, -1 on failure
 * Description:    wl pktq_stats, refer to wl_iov_pktqlog_params
 ******************************************************************/
int
bcm_get_pktq_stats(void *wl, struct wl_iov_mac_params_t *input_params, char *buf)
{
	int ret;
	wl_iov_mac_full_params_t*  full_params = (wl_iov_mac_full_params_t*)buf;
	wl_iov_mac_params_t*       params = &full_params->params;
	wl_iov_mac_extra_params_t* extra_params = &full_params->extra_params;

	wlc_rev_info_t revinfo;
	uint32 corerev;
	
	memset(&revinfo, 0, sizeof(revinfo));
	ret = wlu_get(wl, WLC_GET_REVINFO, &revinfo, sizeof(revinfo));
	if (ret) {
		return -1;
	}
	corerev = dtoh32(revinfo.corerev);

	memset(full_params, 0, sizeof(*full_params));
	memset(&loop_params, 0, sizeof(loop_params));
	memset(&loop_extra_params, 0, sizeof(loop_extra_params));

	// C:  common queue stats
	if (input_params->addr_type[0] == 'C') {
		params->addr_type[0] = input_params->addr_type[0];
		extra_params->addr_info[0] = 0xFFFF;
		params->num_addrs = 1;
		params->num_addrs |= 4 << 8;
		
		if ((ret = wlu_iovar_getbuf(wl, "pktq_stats", full_params,	
				sizeof(*full_params), buf, WLC_IOCTL_MAXLEN)) < 0) {
			return -1;
		}
		/* BRCM print pktq_stats function */
		//wl_txq_macparam_dump((wl_iov_pktq_log_t*)buf, FALSE, corerev >= 40, FALSE); 
		return 0;
	}

	// A: ampdu queue stats
	if (input_params->addr_type[0] == 'A') {
		if (input_params->num_addr == 1) {
			params->num_addrs = 1;
			extra_params->addr_info[0] |= PKTQ_LOG_AUTO;
			params->addr_type[0] = input_params->addr_type[0];
			params->ea[0] = input_params->ea[0];

			/* set a "version" indication */
			params->num_addrs |= 4 << 8;

			if ((ret = wlu_iovar_getbuf(wl, "pktq_stats", full_params,
					sizeof(*full_params), buf, WLC_IOCTL_MAXLEN)) < 0) {
				return -1;
			}
			/* BRCM print pktq_stats function */
			//wl_txq_macparam_dump((wl_iov_pktq_log_t*)buf, TRUE, corerev >= 40, FALSE);
		
		} /*
			else {
				do you need go through assoclist to get all sta ampdu queue stats?
			}
		     */
	}

	return 0;

}
