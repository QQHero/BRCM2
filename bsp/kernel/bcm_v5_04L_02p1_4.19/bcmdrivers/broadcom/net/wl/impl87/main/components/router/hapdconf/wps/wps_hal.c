/*
 * WPS HAL
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
 *
 * $Id: wps_hal.c 773620 2019-03-26 18:31:43Z $
 */

#include <typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmrobo.h>
#include <wps_led.h>
#include <bcmparams.h>
#include <bcmnvram.h>

#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <etioctl.h>

#ifdef __linux__
#include <linux/sockios.h>
#ifndef	u64
typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
#endif
#include <linux/ethtool.h>
#include <sys/time.h>
#include <signal.h>
#endif	/* __linux__ */

#include <shutils.h>
#include <bcmutils.h>
#include <wps_led.h>
#include <wlutils.h>
#include <bcmparams.h>
#include <wps_gpio.h>

/* defines needed */
#define WPS_LED_FLASH_CR		0x10
#define WPS_LEDa_CR			0x12
#define WPS_LEDb_CR			0x14
#define WPS_LEDc_CR			0x16

#define WPS_LED_CR_PAGE0		0x0
#define WPS_LED_CR			WPS_LEDa_CR

#define WPS_LED_MODE_NORMAL		0x3
#define WPS_LED_MODE_FLASH		0x2
#define WPS_LED_MODE_ON			0x1
#define WPS_LED_MODE_OFF		0x0

#define PORT0_SHIFT			0x0
#define PORT1_SHIFT			0x2
#define PORT2_SHIFT			0x4
#define PORT3_SHIFT			0x6
#define PORT4_SHIFT			0x8

#define WPS_LED_MAX_PORTS		5

/* Variables needed for hardward LED */
static int wps_led_port_shift[WPS_LED_MAX_PORTS] =
{
	PORT0_SHIFT,
	PORT1_SHIFT,
	PORT2_SHIFT,
	PORT3_SHIFT,
	PORT4_SHIFT
};

static int wps_led_port_lan[WPS_LED_MAX_PORTS] = {-1};
static int wps_led_lanfd = -1, wps_lan_led = 1;
static char wps_led_lanifname[IFNAMSIZ];

#define WPS_DEF_VLAN_PORTS	"3 2 1 0"	/* 4704 */
#ifdef _TUDEBUGTRACE
#define WPS_LED_ERROR(fmt, arg...) printf(fmt, ##arg)
#define WPS_LED_INFO(fmt, arg...) printf(fmt, ##arg)
#else
#define WPS_LED_ERROR(fmt, arg...)
#define WPS_LED_INFO(fmt, arg...)
#endif

static int func_robord = SIOCGETCROBORD;
static int func_robowr = SIOCSETCROBOWR;

/* IOCTL code */
static int
wps_led_ioctl(int fd, int cmd, void *arg)
{
	int ret;

	/* usr can use wps_lan_led nvram setting to skip wps lan led feature */
	if (wps_lan_led == 0) {
		WPS_LED_INFO("WPS lan led control disabled by nvram wps_lan_led setting\n");
		return -2;
	}

#if defined(__linux__)

	ret = ioctl(fd, cmd, arg);

#endif /* __linux__ */

	return ret;
}

static void
getlanports(char *vlan0ports)
{
	int i;
	char port[8], *next;

	for (i = 0; i < WPS_LED_MAX_PORTS; i++)
		wps_led_port_lan[i] = i;

	/* get vlan0ports */
	i = 0;
	if (vlan0ports) {
		foreach(port, vlan0ports, next) {
			if (strchr(port, 't') ||
				strchr(port, '*') ||
				strchr(port, 'u'))
				continue;

			if (i < WPS_LED_MAX_PORTS)
				wps_led_port_lan[i++] = atoi(port);
		}

		for (; i < WPS_LED_MAX_PORTS; i++)
			wps_led_port_lan[i] = -1;
	}

	return;
}

/* LAN LEDs Partion */
static int
lanportmap(int port)
{
	if (port >= 0 && port < WPS_LED_MAX_PORTS) {
		if (wps_led_port_lan[port] != -1)
			return wps_led_port_lan[port];
	}

	return -1;
}

static int
wps_led_lan_mode(int ledcr, int mode, unsigned char portmap[])
{
	struct ifreq ifr;
	int i, index, args[2];
	int val = 0, ret = -1;

	if (wps_led_lanfd >= 0) {
		strncpy(ifr.ifr_name, wps_led_lanifname, sizeof(ifr.ifr_name));
		ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
		args[0] = WPS_LED_CR_PAGE0 << 16;
		if (ledcr == -1) /* use default LEDa */
			args[0] |= WPS_LED_CR & 0xffff;
		else
			args[0] |= ledcr & 0xffff;
		ifr.ifr_data = (caddr_t) args;

		/* get current value */
		if ((ret = wps_led_ioctl(wps_led_lanfd, func_robord, (void*)&ifr)) < 0) {
			WPS_LED_ERROR("%s: Get LAN leds mode failed\n", wps_led_lanifname);
			return -1;
		}

		for (i = 0; i < WPS_LED_MAX_PORTS; i++) {
			index = lanportmap(i);
			if (index >= 0) {
				/* clear val bits first */
				val = 0x3 << wps_led_port_shift[index];
				args[1] &= ~val;

				if (isset(portmap, i))
					val = mode << wps_led_port_shift[index];
				else
					val = WPS_LED_MODE_OFF << wps_led_port_shift[index];

				/* set new value */
				args[1] |= val;
			}
		}
		args[1] &= 0xffff;
		if ((ret = wps_led_ioctl(wps_led_lanfd, func_robowr, (void*)&ifr)) < 0) {
			WPS_LED_ERROR("%s: Set LAN leds mode failed\n", wps_led_lanifname);
		}
	}

	return ret;
}

/* Set all LAN leds */
static int
wps_led_lan_all(int mode, unsigned char portmap[])
{
	/* set all port LEDa, LEDb, LEDc mode to NORMAL */
	int ret = -1;

	if (wps_led_lanfd >= 0) {
		/* LEDa */
		ret = wps_led_lan_mode(WPS_LEDa_CR, mode, portmap);
		/* LEDb */
		ret |= wps_led_lan_mode(WPS_LEDb_CR, mode, portmap);
		/* LEDc */
		ret |= wps_led_lan_mode(WPS_LEDc_CR, mode, portmap);
	}

	return ret;
}

static void
wps_led_lan_support_filter()
{
	char *str = NULL;
	struct ifreq ifr;
	uint devid;
	et_var_t var;

	/* get wps_lan_led nvram value to enable(not 0)/disable(0) wps lan led feature */
	if ((str = nvram_get("wps_lan_led")))
		wps_lan_led = atoi(str);

	/* user disabled wps lan led feature */
	if (!wps_lan_led)
		return;

	/* check is the switch we can support */
	strncpy(ifr.ifr_name, wps_led_lanifname, sizeof(ifr.ifr_name));
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
	memset(&var, 0, sizeof(var));
	var.cmd = IOV_ET_ROBO_DEVID;
	var.buf = &devid;
	var.len = sizeof(devid);
	ifr.ifr_data = (caddr_t)&var;

	/* get device ID */
	if (wps_led_ioctl(wps_led_lanfd, SIOCSETGETVAR, (void*)&ifr) < 0) {
		WPS_LED_ERROR("%s: Get switch device ID failed, disable wps lan led feature\n",
			wps_led_lanifname);
		wps_lan_led = 0;
		return;
	}

	/* filter it */
	WPS_LED_INFO("%s: Switch Device ID =0x%x\n", wps_led_lanifname, devid);
	switch (devid) {
	case DEVID5325:
		break;

	case DEVID5395:
	case DEVID5397:
	case DEVID5398:
	case DEVID53115:
	case DEVID53125:
	default:
		wps_lan_led = 0;
		WPS_LED_INFO("Unsupported device ID [0x%x] for software control, "
			"force to disable wps lan led feature!\n", devid);
		break;
	}

	return;
}

static int
wps_led_lan_init(char *ifname, char *vlan0ports)
{
	if (ifname == NULL) {
		WPS_LED_ERROR("No LAN interface name\n");
		return -1;
	}

	if (vlan0ports == NULL) {
		WPS_LED_ERROR("No vlan0ports information\n");
		return -1;
	}

	WPS_LED_INFO("ifname = %s, vlan0ports = %s\n", ifname, vlan0ports);

	strncpy(wps_led_lanifname, ifname, sizeof(wps_led_lanifname));
	wps_led_lanifname[sizeof(wps_led_lanifname) - 1] = '\0';

	if ((wps_led_lanfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		WPS_LED_ERROR("Create LAN leds control socket failed\n");
		return -1;
	}

	/* retrieve lan ports number */
	getlanports(vlan0ports);

	/* filter wps lan led support */
	wps_led_lan_support_filter();

	return 0;
}

static void
wps_led_lan_cleanup()
{
	if (wps_led_lanfd >= 0) {
		close(wps_led_lanfd);
		wps_led_lanfd = -1;
	}
}

/*
 * OS support portion of ioctl and timer function
 */
#if defined(__linux__)
static int
wps_hal_etcheck(int s, struct ifreq *ifr)
{
	struct ethtool_drvinfo info;

	memset(&info, 0, sizeof(info));
	info.cmd = ETHTOOL_GDRVINFO;
	ifr->ifr_data = (caddr_t)&info;
	if (ioctl(s, SIOCETHTOOL, (caddr_t)ifr) < 0) {
		/* print a good diagnostic if not superuser */
		return -1;
	}

	if (!strncmp(info.driver, "et", 2))
		return 0;
	else if (!strncmp(info.driver, "bcm57", 5))
		return 0;

	return -1;
}

static int
wps_hal_get_etname(char *etName)
{
	int s;
	struct ifreq ifr;
	FILE *fp;
	char buf[512], *c, *name;
	char proc_net_dev[] = "/proc/net/dev";

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	ifr.ifr_name[0] = '\0';

	/* eat first two lines */
	if (!(fp = fopen(proc_net_dev, "r")) ||
	        !fgets(buf, sizeof(buf), fp) ||
	        !fgets(buf, sizeof(buf), fp)) {
		if (fp)
			fclose(fp);
		close(s);
		return -1;
	}

	while (fgets(buf, sizeof(buf), fp)) {
		c = buf;
		while (isspace(*c))
			c++;
		if (!(name = strsep(&c, ":")))
			continue;
		strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = 0;
		if (wps_hal_etcheck(s, &ifr) == 0)
			break;
		ifr.ifr_name[0] = '\0';
	}

	fclose(fp);
	close(s);

	if (!*ifr.ifr_name)
		return -1;

	strcpy(etName, ifr.ifr_name);
	return 0;
}

static void
wps_hal_led_timer_init()
{
	struct itimerval tv;
	struct sigaction sact;

	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = wps_gpio_led_blink_timer;
	sigaction(SIGALRM, &sact, NULL);

	tv.it_interval.tv_sec = 0;
	tv.it_interval.tv_usec = WPS_LED_BLINK_TIME_UNIT * 1000;
	tv.it_value.tv_sec = 0;
	tv.it_value.tv_usec = WPS_LED_BLINK_TIME_UNIT * 1000;

	setitimer(ITIMER_REAL, &tv, NULL);
}

static void
wps_hal_led_timer_cleanup()
{
	struct itimerval tv;

	tv.it_interval.tv_sec = 0;
	tv.it_interval.tv_usec = 0;
	tv.it_value.tv_sec = 0;
	tv.it_value.tv_usec = 0;

	setitimer(ITIMER_REAL, &tv, NULL);
}
#endif	/* __linux__ */

/*
 * LED HAL public functions
 */
int
wps_hal_led_init()
{
	int unit;
	char *next, *etname = NULL;
	char name[IFNAMSIZ], tmp[100];
	char landev[16], *landevs;
	char vport[16], *vports, *vnext, allvports[256];
	int ret;

	memset(name, 0, sizeof(name));
	if (wps_hal_get_etname(name) == 0)
		etname = name;

	/* get all vlanxports informations */
	memset(allvports, 0, sizeof(allvports));
	if ((landevs = nvram_get("landevs"))) {
		foreach(landev, landevs, next) {
			if (strncmp(landev, "vlan", 4) == 0) {
				if (get_ifname_unit(landev, &unit, NULL) == -1)
					continue;
				snprintf(tmp, sizeof(tmp), "vlan%dports", unit);
				if ((vports = nvram_get(tmp))) {
					foreach(vport, vports, vnext) {
						add_to_list(vport, allvports, sizeof(allvports));
					}
				}
			}
		}
	}

	/* set default ports when no any vlanxports config exist */
	if (!strlen(allvports))
		strcpy(allvports, WPS_DEF_VLAN_PORTS);

	/* initial led control through GPIO */
	ret = wps_gpio_led_init();

	/* initial LAN leds control through software mode */
	if (!ret && etname) {
		ret = wps_led_lan_init(etname, allvports);
	}

	/* initial osl gpio led blinking timer */
	wps_hal_led_timer_init();

	return ret;
}

void
wps_hal_led_deinit()
{
	wps_gpio_led_cleanup();
	wps_led_lan_cleanup();
	wps_hal_led_timer_cleanup();
}

void
wps_hal_led_blink(wps_blinktype_t blinktype)
{
	wps_gpio_led_blink(blinktype);
}

/* HAL potion wl instance selection by push button */
static int
wps_led_lan_flash_time(int rate)
{
	struct ifreq ifr;
	int args[2], ret = -1;

	if (wps_led_lanfd >= 0) {
		strncpy(ifr.ifr_name, wps_led_lanifname, sizeof(ifr.ifr_name));
		ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
		args[0] = WPS_LED_CR_PAGE0 << 16;
		args[0] |= WPS_LED_FLASH_CR & 0xffff;
		args[1] = rate;
		ifr.ifr_data = (caddr_t) args;

		if ((ret = wps_led_ioctl(wps_led_lanfd, func_robowr, (void*)&ifr)) < 0) {
			WPS_LED_ERROR("%s: Set LAN leds flash rate failed\n", wps_led_lanifname);
		}
	}

	return ret;
}

int
wps_hal_led_wl_select_on()
{
	int i;
	unsigned char portmap[(WPS_LED_MAX_PORTS+7) / 8] = {0};

	/* set LEDa, LEDb, LEDc to NORMAL, flash time to default 0x2 */
	wps_led_lan_flash_time(WPS_LED_FLASH_DEFAULT);
	for (i = 0; i < WPS_LED_MAX_PORTS; i++)
		setbit(portmap, i);

	wps_led_lan_all(WPS_LED_MODE_OFF, portmap);

	return 0;
}

int
wps_hal_led_wl_select_off()
{
	int i;
	unsigned char portmap[(WPS_LED_MAX_PORTS+7) / 8] = {0};

	/* set LEDa, LEDb, LEDc to NORMAL, flash time to default 0x2 */
	wps_led_lan_flash_time(WPS_LED_FLASH_DEFAULT);
	for (i = 0; i < WPS_LED_MAX_PORTS; i++)
		setbit(portmap, i);
	wps_led_lan_all(WPS_LED_MODE_NORMAL, portmap);

	return 0;
}

void
wps_hal_led_wl_selecting(int led_id)
{
	unsigned char portmap[(WL_MAXBSSCFG+7) / 8] = {0};

	/* Flash this led */
	wps_led_lan_flash_time(WPS_LED_FLASH_FAST);
	setbit(portmap, led_id);
	wps_led_lan_mode(-1, WPS_LED_MODE_FLASH, portmap);
}

void
wps_hal_led_wl_confirmed(int led_id)
{
	unsigned char portmap[(WL_MAXBSSCFG+7) / 8] = {0};

	/* Flash this led */
	wps_led_lan_flash_time(WPS_LED_FLASH_SLOW);
	setbit(portmap, led_id);
	wps_led_lan_mode(-1, WPS_LED_MODE_FLASH, portmap);
}

int
wps_hal_led_wl_max()
{
	return (WPS_LED_MAX_PORTS - 1);
}

/*
 * Push button HAL public functions
 */
wps_btnpress_t
wps_hal_btn_pressed()
{
	return wps_gpio_btn_pressed();
}

int
wps_hal_btn_init()
{
	return wps_gpio_btn_init();
}

void
wps_hal_btn_cleanup()
{
	wps_gpio_btn_cleanup();
}
