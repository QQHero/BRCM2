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
# $Id $
#

UPNPLIB = $(SRCBASE_ROUTER)/libupnp
SRCPATH = $(SRCBASE_ROUTER)/igd
DEVPATH = $(SRCPATH)/InternetGatewayDevice
OSLPATH = $(SRCPATH)/linux
OBJPATH = obj

INCLUDES += -I$(SRCPATH) -I$(DEVPATH) -I$(UPNPLIB)/include -I$(SRCBASE)/include ${WLAN_StdIncPathA} -I$(SRCBASE)/../components/shared -I$(SRCBASE)/../components/wlioctl/include -I$(SRCBASE)/../components/proto/include -I$(SRCBASE)/common/include -I$(SRCBASE_ROUTER)/shared

CFLAGS += $(INCLUDES)
CFLAGS += -Wall -Wunused -g -Werror

LDFLAGS += -L$(TOP)/libbcmcrypto -L$(INSTALLDIR)/libbcmcrypto/usr/lib -lbcmcrypto
LDFLAGS += -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram
LDFLAGS += -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared -lpthread
LDFLAGS += -L$(TOP)/libupnp -L$(INSTALLDIR)/libupnp/usr/lib -lupnp
LDFLAGS += $(EXTRA_LDFLAGS)

SRCFILES = \
	igd_linux_main.c \
	igd_linux_osl.c \
	igd_mainloop.c \
	InternetGatewayDevice_table.c \
	InternetGatewayDevice.c \
	soap_x_layer3forwarding.c \
	soap_x_wancommoninterfaceconfig.c \
	soap_x_wanipconnection.c \
	xml_InternetGatewayDevice.c \
	xml_x_layer3forwarding.c \
	xml_x_wancommoninterfaceconfig.c \
	xml_x_wanipconnection.c

VPATH = $(SRCPATH):$(OSLPATH):$(DEVPATH)

OBJFILES := $(patsubst %.c,$(OBJPATH)/%.o,$(SRCFILES))

#
# Rules
#
TARGET = igd

all : $(OBJPATH) $(TARGET)

$(OBJPATH):
	mkdir -p $(OBJPATH)

$(OBJPATH)/%.o:	%.c | $(OBJPATH)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

$(TARGET) :: $(OBJFILES)
	$(CC) -o $@ $(OBJFILES) $(LDFLAGS)

install:
	install -d $(INSTALLDIR)/usr/sbin
	install -m 755 $(TARGET) $(INSTALLDIR)/usr/sbin/$(TARGET)
	$(STRIP) $(INSTALLDIR)/usr/sbin/$(TARGET)

clean:
	rm -rf $(OBJPATH)
	rm -f $(TARGET)

FORCE:

.PHONY: FORCE clean install
