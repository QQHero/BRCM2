#
# Hostapd Configuration Utility Makefile
#
# Copyright 2022 Broadcom
#
# This program is the proprietary software of Broadcom and/or
# its licensors, and may only be used, duplicated, modified or distributed
# pursuant to the terms and conditions of a separate, written license
# agreement executed between you and Broadcom (an "Authorized License").
# Except as set forth in an Authorized License, Broadcom grants no license
# (express or implied), right to use, or waiver of any kind with respect to
# the Software, and Broadcom expressly reserves all rights in and to the
# Software and all intellectual property rights therein.  IF YOU HAVE NO
# AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
# WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
# THE SOFTWARE.
#
# Except as expressly set forth in the Authorized License,
#
# 1. This program, including its structure, sequence and organization,
# constitutes the valuable trade secrets of Broadcom, and you shall use
# all reasonable efforts to protect the confidentiality thereof, and to
# use this information only in connection with your use of Broadcom
# integrated circuit products.
#
# 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
# "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
# REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
# OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
# DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
# NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
# ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
# OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
#
# 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
# BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
# SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
# IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
# IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
# ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
# OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
# NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
#
# $Id: $
#

CFLAGS += $(USR_CFLAGS) -I$(WLSRCBASE)/include -I$(ALLSRCBASE)/$(HNDDRIVER_BASE)/shared -I$(ALLSRCBASE)/main/components/wlioctl/include -I$(ALLSRCBASE)/main/components/proto/include -I$(WLSRCBASE)/shared/bcmwifi/include -I$(ALLSRCBASE)/main/components/bcmcrypto/include

LDFLAGS += -L$(INSTALL_DIR)/lib -L$(INSTALL_DIR)/usr/lib -lnvram -lshared -lbcmcrypto -lwlcsm -lpthread

#for wps_pbcd
OPENSOURCE_BASE_DIR := $(ALLSRCBASE)/main/components/opensource
HOSTAPD_DIR := $(OPENSOURCE_BASE_DIR)/router_tools/hostapd
WPA_SUPPLICANT_DIR := $(OPENSOURCE_BASE_DIR)/router_tools/wpa_supplicant

CFLAGS += -Iwps/include -I$(WPA_SUPPLICANT_DIR)/src/common -I$(WL_CMWIFI_DIR)/libs/wlcsm/include -I$(ALLSRCBASE)/main/components/router/bcmdrv/include -I$(ALLSRCBASE)/$(HNDDRIVER_BASE)/rc

LDFLAGS += -L$(WPA_SUPPLICANT_DIR)/wpa_supplicant -lwpa_client -lbcm
LDFLAGS += -L$(LIBNV2HAPDCFG_DIR) -lnv2hapdcfg

LIBNV2HAPDCFG_DIR = ../rc
LIBNV2HAPDCFG = libnv2hapdcfg.so
OBJS_HAPD_CONF := main.o
OBJS_WPS_PBCD := wps_pbcd.o wps_hal.o wps_gpio.o
ifneq ($(strip $(BUILD_RDKWIFI)),)
CFLAGS += -DCONFIG_HOSTAPD
LDFLAGS += $(EXTRA_LDFLAGS)
ALL := hapd_conf
else
ALL := hapd_conf wps_pbcd
endif

all: $(ALL)

$(LIBNV2HAPDCFG):
	$(MAKE) -C $(LIBNV2HAPDCFG_DIR) $@

hapd_conf: $(LIBNV2HAPDCFG) $(OBJS_HAPD_CONF)
	$(CC) -o $@ $(OBJS_HAPD_CONF) $(LDFLAGS)

wps_pbcd: $(LIBNV2HAPDCFG) $(OBJS_WPS_PBCD)
	$(CC) -o $@ $(OBJS_WPS_PBCD) $(LDFLAGS)

install: all
	install -m 755 hapd_conf $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/hapd_conf
ifeq ($(strip $(BUILD_RDKWIFI)),)
	install -m 755 $(LIBNV2HAPDCFG_DIR)/$(LIBNV2HAPDCFG) $(INSTALL_DIR)/lib
	cd $(INSTALL_DIR)/bin && ln -sf hapd_conf erase
	install -m 755 wps_pbcd $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/wps_pbcd
	cd $(INSTALL_DIR)/bin && ln -sf wps_pbcd erase
endif

clean:
	rm -f *.o hapd_conf wps_pbcd

hostapd_config.o: $(ALLSRCBASE)/$(HNDDRIVER_BASE)/rc/hostapd_config.c
	$(CC) -c $(CFLAGS) -o $@ $(ALLSRCBASE)/$(HNDDRIVER_BASE)/rc/hostapd_config.c

wps_pbcd.o: $(ALLSRCBASE)/$(HNDDRIVER_BASE)/rc/wps_pbcd.c
	$(CC) -c $(CFLAGS) -o $@ $(ALLSRCBASE)/$(HNDDRIVER_BASE)/rc/wps_pbcd.c

wps_hal.o: wps/wps_hal.c
	$(CC) -c $(CFLAGS) -o $@ wps/wps_hal.c

wps_gpio.o: wps/wps_gpio.c
	$(CC) -c $(CFLAGS) -o $@ wps/wps_gpio.c

main.o: main.c
	$(CC) -c $(CFLAGS) -o $@ main.c
