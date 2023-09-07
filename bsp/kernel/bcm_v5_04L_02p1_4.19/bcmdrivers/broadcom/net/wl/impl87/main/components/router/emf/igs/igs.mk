#
# Copyright (C) 2022, Broadcom. All Rights Reserved.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
# OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# $Id: igs.mk 803553 2021-09-29 11:42:23Z $
#

CROSS_COMPILE = mipsel-linux-
KSRC := $(shell /bin/pwd)/../../..
KINCLUDE = -I$(KSRC)/include -I$(LINUXDIR)/include/asm/gcc \
	   -I$(KSRC)/router/emf/igs -I$(KSRC)/router/emf/emf

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

OFILES = igsc.o igs_linux.o igsc_sdb.o osl_linux.o

IFLAGS1 = -I$(LINUXDIR)/include
IFLAGS2 = $(KINCLUDE)
DFLAGS += -DMODULE -D__KERNEL__ $(IFLAGS) -G 0 -mno-abicalls \
	  -fno-pic -pipe -gstabs+ -mcpu=r4600 -mips2 -Wa,--trap \
	  -m4710a0kern -mlong-calls -fno-common -nostdinc \
	  -iwithprefix include

WFLAGS = $(IFLAGS1) -Wall -Wstrict-prototypes -Wno-trigraphs -O2 \
	 -fno-strict-aliasing -fno-common -fomit-frame-pointer $(IFLAGS2)

CFLAGS += $(WFLAGS) $(DFLAGS) $(IGSFLAGS)

TARGETS = igs.o

all: $(TARGETS)

igs.o: $(OFILES)
	$(LD) -r -o $@ $(OFILES)

clean:
	rm -f $(OFILES) $(TARGETS) *~

.PHONY: all clean

include $(LINUXDIR)/Rules.make
