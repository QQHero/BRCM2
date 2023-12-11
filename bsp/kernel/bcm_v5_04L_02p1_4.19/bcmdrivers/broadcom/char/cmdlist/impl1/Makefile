
# File: bcmdrivers/broadcom/char/cmdlist/impl1/Makefile
#
# Makefile for the Command List driver
#
#***********************************************************************
#
#  Copyright (c) 2006-2017  Broadcom Corporation
#  All Rights Reserved
#
# <:label-BRCM:2017:proprietary:standard
# 
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
# 
#  Except as expressly set forth in the Authorized License,
# 
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
# 
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
# 
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>
#
#***********************************************************************/

cmdlist-objs :=  cmdlist_driver.o cmdlist_ucast.o cmdlist_l2_ucast.o cmdlist_mcast.o

ifneq ($(strip $(CONFIG_BCM_RDPA)),)
# Runner
ifneq ($(strip $(CONFIG_BCM_CMDLIST_GPE)),)
cmdlist-objs += gpe_api.o
ifneq ($(strip $(CONFIG_BCM_CMDLIST_SIM)),)
cmdlist-objs += gpe_sim.o
cmdlist-objs += runner_sim.o
endif
endif
ifneq ($(strip $(CONFIG_BCM_CMDLIST_XPE)),)
cmdlist-objs += xpe_api.o
ifneq ($(strip $(CONFIG_BCM_CMDLIST_SIM)),)
cmdlist-objs += xpe_sim.o
cmdlist-objs += xpe_runner_sim.o
endif
endif
endif

ifneq ($(strip $(CONFIG_BCM_ARCHER)),)
# System Port
cmdlist-objs += spe_api.o
endif

obj-$(CONFIG_BCM_CMDLIST) += cmdlist.o

EXTRA_CFLAGS += -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/enet/shared
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/archer/impl1/
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)/pmc
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += $(INC_RDP_FLAGS)
EXTRA_CFLAGS += -I$(INC_RDPA_MW_PATH)

EXTRA_CFLAGS += -Werror -Wall -Wfatal-errors 

ifneq ($(strip $(CONFIG_BCM_DHD_RUNNER)),)
EXTRA_CFLAGS += -DCONFIG_DHD_RUNNER
endif

clean:
	rm -f core *.o *.a *.s

# Makefile trick: breaking the following into two rules allows
# the "%.o : %.c" rule (defined earlier by kbuild), to take precidence

%.o : %.o_tmp
	$(Q)mv $< $@

ifeq ("$(CONFIG_BRCM_CHIP_REV)","0x4912A0")
%.o_tmp : %.$(BCMARCH)_4912A0.o_saved
else ifeq ("$(CONFIG_BRCM_CHIP_REV)","0x6813A0")
%.o_tmp : %.$(BCMARCH)_6813A0.o_saved
else ifeq ("$(CONFIG_BRCM_CHIP_REV)","0x63146A0")
%.o_tmp : %.$(BCMARCH)_63146A0.o_saved
else
%.o_tmp : %.$(BCMARCH)_$(BRCM_CHIP).o_saved
endif
	@echo "Using $<"
	$(Q)cp $< $@
