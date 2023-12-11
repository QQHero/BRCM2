#
# Linux ssd Makefile
#
# Copyright (C) 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# $Id: Makefile $
#

CFLAGS  += -I. -I$(TOP)/shared -I$(SRCBASE)/include -I$(SRCBASE)/../components/shared -I$(SRCBASE)/../components/wlioctl/include
CFLAGS  += -I$(SRCBASE)/../components/proto/include -I$(SRCBASE)/common/include -Wall -Werror

CFLAGS	+= $(if $(WLAN_ComponentIncPathA),$(WLAN_ComponentIncPathA),$(addprefix -I,$(wildcard $(SRCBASE)/shared/bcmwifi/include)))
# CFLAGS	+= -DSSD_DEBUG
CFLAGS	+= -s -O2 -Wno-stringop-truncation

LDFLAGS += -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram
LDFLAGS += -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared -lpthread
LDFLAGS += $(EXTRA_LDFLAGS)

ifneq ($(CMWIFI),)
LDFLAGS += -L$(INSTALL_DIR)/lib -lwlcsm -lgcc_s
endif # CMWIFI

vpath %.c $(SRCBASE)/shared $(WLAN_ComponentSrcDirsA)

all: ssd

OBJS = ssd.o

install: all
	install -d $(INSTALLDIR)/usr/sbin/
	install ssd $(INSTALLDIR)/usr/sbin/
	$(STRIP) $(INSTALLDIR)/usr/sbin/ssd
	ln -sf ssd $(INSTALLDIR)/usr/sbin/ssd_cli

clean:
	rm -f *.o *.a *.so ssd

${OBJS}: ssd.h

ssd: ${OBJS}
	$(CC) $(CFLAGS) $^ -o $@  $(LDFLAGS)
