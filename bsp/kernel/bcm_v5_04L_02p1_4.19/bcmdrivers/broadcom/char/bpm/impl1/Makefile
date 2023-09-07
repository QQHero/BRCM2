# File: bcmdrivers/char/bpm/bcm963xx/Makefile
#
# Makefile for the BCM63xx BPM module
#

bcm_bpm-objs := bpm.o

ifneq ($(strip $(CONFIG_BCM_MPM_OVER_BPM)),)
bcm_bpm-objs += bpm_mpm.o
else
bcm_bpm-objs += bpm_sw.o
ifneq ($(strip $(CONFIG_BCM_BPM_BUF_TRACKING)),)
bcm_bpm-objs += bpm_track.o
endif
endif

obj-$(CONFIG_BCM_BPM) += bcm_bpm.o

EXTRA_CFLAGS += -I. -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD) -I$(BRCMDRIVERS_DIR)/opensource/net/enet/shared

EXTRA_CFLAGS += -Werror -Wfatal-errors 

clean:
	rm -f core *.ko *.o *.a *.s

