#
# Linux WLDM Makefile
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
# <<Broadcom-WL-IPTag/Proprietary:>>
#
# $Id: $
#

#PROGS		= wldm

INSTDIR		= $(INSTALLDIR)/wldm/
INSTMODE	= 0755
INSTOWNER	= root
INSTGROUP	= root

WLDM_LIB	= libwldm

LIB_OBJS	+= wldm_lib.o wldm_lib_wifi.o bcm_app_utils.o bcm_math.o

ifneq ($(PROGS),)
LIB_OBJS	+= wldm_lib_tr181.o
OBJS		= wldm.o
endif

ifeq ($(WLDM_LIB),)
OBJS		+= wl.o wl_linux.o shutils.o
OBJS		+= $(LIB_OBJS)
endif

LDLIBS		+= -lrt -lpthread
LDLIBS		+= -L$(TOP)/shared -lshared -lwlcsm -lnvram

MAX_THREADS	= 32
ifneq ($(MAX_THREADS),)
CFLAGS		+= -DWLDM_MAX_THREADS=$(MAX_THREADS)
endif

CC		?= $(CROSS_COMPILE)gcc
INSTALL		= install

all: $(WLDM_LIB) $(PROGS)

CFLAGS		+= -DSHELLPATH=\"$(run_prefix)/bin/sh\" -Wall -Os -fPIC -Werror
CFLAGS		+= -I$(ALLSRCBASE)/main/src/include
CFLAGS		+= -I$(ALLSRCBASE)/main/src/shared/bcmwifi/include
CFLAGS		+= -I$(ALLSRCBASE)/main/components/proto/include
CFLAGS		+= -I$(ALLSRCBASE)/main/components/router/shared
CFLAGS		+= -I$(ALLSRCBASE)/main/components/wlioctl/include
CFLAGS		+= -I$(ALLSRCBASE)/sys/components/apps/hspot/router/shared/
CFLAGS		+= -I$(ALLSRCBASE)/main/components/math/include
CFLAGS		+= -I$(WL_CMWIFI_DIR)/libs/wlcsm/include

# For feat_bca_wlan
CFLAGS		+= -I$(ALLSRCBASE)/main/src/router/shared -I$(ALLSRCBASE)/main/src/common/include
CFLAGS		+= -I$(WL_RG_APPS_PRIVATE_DIR)/libs/wlcsm/include

CFLAGS		+= $(USR_CFLAGS)

LDFLAGS		+= -L$(INSTDIR)/lib $(EXTRA_LDFLAGS)

ifneq ($(BUILD_RDKWIFI),)
CFLAGS += -DBCA_CPEROUTER_RDK
endif

ifneq ("","$(DEBUG)")
CFLAGS		+= -DDEBUG -g
endif

vpath %.c $(SRCBASE_ROUTER)/shared/
vpath %.c $(ALLSRCBASE)/main/src/router/shared/
vpath %.c $(ALLSRCBASE)/main/src/shared/
vpath %.c $(ALLSRCBASE)/main/components/math/src/

# WLDM lib
.PHONY: libwldm
libwldm: $(LIB_OBJS)
	#$(CC) -o $@.a $(LIB_OBJS)
ifneq ($(BUILD_RDKWIFI),)
	$(CC) -shared -o $@.so $(LIB_OBJS) $(LDLIBS) -L$(INSTALL_DIR)/lib -L$(INSTALLDIR)/shared/usr/lib
else
	$(CC) -shared -o $@.so $(LIB_OBJS) $(LDLIBS) -L$(INSTDIR)/lib
endif
	$(INSTALL) -d $(INSTDIR)/lib
	$(INSTALL) -m 755 $@.so $(INSTDIR)/lib
	$(STRIP) $(INSTDIR)/lib/$@.so

# WLDM client app
$(PROGS): $(WLDM_LIB) $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) $(WLDM_LIB:lib%=-l%) -o $@
	$(INSTALL) -d $(INSTDIR)/sbin
	$(INSTALL) -m $(INSTMODE) $(PROGS) $(INSTDIR)/sbin
	$(STRIP) $(foreach prog,$(PROGS),$(INSTDIR)/sbin/$(prog))

.PHONY: install
install: all
	mkdir -p $(INSTALL_DIR)/etc/wlan/

.PHONY: clean
clean:
	rm -f $(PROGS) *.*o
