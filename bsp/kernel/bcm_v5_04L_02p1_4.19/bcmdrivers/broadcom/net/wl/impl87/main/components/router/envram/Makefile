#
# Linux ENVRAM Makefile
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
# $Id: Makefile 679153 2017-01-12 19:36:34Z $
#

include $(TOP)/.config

CFLAGS	+= -I. -I$(SRCBASE)/include ${WLAN_StdIncPathA} -I$(SRCBASE)/../components/shared -I$(SRCBASE)/../components/wlioctl/include -I$(SRCBASE)/../components/proto/include -I$(SRCBASE)/../components/math/include -I$(SRCBASE)/common/include -I$(SRCBASE)/tools/misc//lzma_src/C/ -Wall

#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
ifneq (2_4,$(LINUX_VERSION))
CFLAGS += -DLINUX26
endif
LDFLAGS += -L.

vpath %.c $(SRCBASE)/shared/ $(SRCBASE)/tools/misc/lzma_src/C/ $(SRCBASE)/../components/math/src

all: $(if $(CONFIG_ENVRAM_UTILITY),envrams) $(if $(CONFIG_ENVRAM_UTILITY),envram)

install: all
ifneq ($(CONFIG_ENVRAM_UTILITY),)
	install -d $(INSTALLDIR)/usr/sbin/
	install envrams $(INSTALLDIR)/usr/sbin/
	install envram $(INSTALLDIR)/usr/sbin/
	$(STRIP) $(INSTALLDIR)/usr/sbin/envrams
	$(STRIP) $(INSTALLDIR)/usr/sbin/envram
else
	# Utility not wanted: get rid of any leftover one
	rm -f $(INSTALLDIR)/usr/sbin/envrams
	rm -f $(INSTALLDIR)/usr/sbin/envram
endif

clean:
	rm -f *.o *.a *.so envram

envrams: bcmutils.o bcm_math.o envrams.o LzmaLib.o Alloc.o LzmaEnc.o LzmaDec.o LzFind.o
	$(CC) $(CFLAGS) $^ -o $@  $(LDFLAGS)

envram: envramc.o
	$(CC) $(CFLAGS) $^ -o $@  $(LDFLAGS)
