# File: bcmdrivers/char/pwrmngt/bcm963xx/Makefile
#
# Makefile for the Power Management Module
#

pwrmngtd-objs   := pwrmngtdrv.o
pwrmngtd-objs   += pwrmngt.o

ifneq ($(strip $(CONFIG_BCM_BMU)),)
pwrmngtd-objs   += bmu.o    
endif

obj-$(CONFIG_BCM_PWRMNGT) += pwrmngtd.o

EXTRA_CFLAGS += -I.
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/net/enet/shared 
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) 
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)/pmc 
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -fno-exceptions
EXTRA_CFLAGS += -Werror -Wfatal-errors

 #-include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s avs*/*.o *.ko

