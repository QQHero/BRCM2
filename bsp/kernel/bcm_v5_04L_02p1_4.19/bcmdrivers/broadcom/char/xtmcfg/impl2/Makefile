# File: bcmdrivers/char/xtmcfg/bcm963xx/Makefile
#
# Makefile for the BCM63xx bcmxtmcfg driver.
#

obj-$(CONFIG_BCM_XTMCFG) += bcmxtmcfg.o
bcmxtmcfg-objs := xtmcfgdrv.o xtmcfgmain.o xtmprocessor.o xtminterface.o xtmconnection.o xtmoamhandler.o xtmoslinux.o xtmasmhandler.o xtmautosense.o

EXTRA_CFLAGS += ${DSL_XTM_SAR_TX_RX_IUDMA_631XX_DEFINES}

EXTRA_CFLAGS += -DCONFIG_$(BRCM_CHIP) -fno-exceptions -fno-rtti -I. -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_ADSLDRV_PATH) -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)/pmc
EXTRA_CFLAGS += -Werror -Wfatal-errors
#EXTRA_CFLAGS += -g

ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63268_63138_63381_63148_63158_63178_63146_),_$(strip $(BRCM_CHIP))_)
EXTRA_CFLAGS += -DCONFIG_VDSL_SUPPORTED
endif

ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63158_63146_),_$(strip $(BRCM_CHIP))_)
EXTRA_CFLAGS += -DXTM_PORT_SHAPING -DHW_SAR_BASED_PORT_SHAPING_SUPPORT
endif

# BCM6315X following feature is disabled by default. Please consult with BRCM
# before enabling the following switch. Also this switch can be enabled only
# for experimental purposes. Refer to 6315X errata for further details on the
# port shaping support.
ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63138_63148_63158_),_$(strip $(BRCM_CHIP))_)
EXTRA_CFLAGS += -DDXTM_BOND_24HDR
endif

ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63158_),_$(strip $(BRCM_CHIP))_)
EXTRA_CFLAGS += -DDXTM_6315X_GFAST_PORT_SHAPING_SUPPORTED
endif

ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63146_),_$(strip $(BRCM_CHIP))_)
EXTRA_CFLAGS += -DHW_PTM_TX_BONDING
endif

EXTRA_CFLAGS += ${BUILD_DSL_FLAGS}

ifeq ($(strip $(BRCM_DRIVER_ADSL)),)
EXTRA_CFLAGS += -DNO_XDSL_MODULE
endif

EXTRA_CFLAGS += -fno-store-merging

# no-rtti is not applicable to c files
STRIP_FROM_C := -fno-rtti

# Some compiler flags which are defined at the top level Makefile are not
# applicable to C++ files.  Filter them out to avoid warnings.  This is a
# brute force approach, but is safer than modifying the top level Makefiles.
STRIP_FROM_CPP := -Wstrict-prototypes -Werror-implicit-function-declaration \
                  -Wdeclaration-after-statement -Wno-pointer-sign -std=gnu89 -std=gnu90

CFLAGS_REMOVE_xtmcfgdrv.o := $(STRIP_FROM_C)
CFLAGS_REMOVE_xtmoslinux.o := $(STRIP_FROM_C)
CFLAGS_REMOVE_xtmcfgmain.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmprocessor.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtminterface.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmconnection.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmoamhandler.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmasmhandler.o := $(STRIP_FROM_CPP)
CFLAGS_REMOVE_xtmautosense.o := $(STRIP_FROM_CPP)


clean:
	rm -f core *.o *.a *.s

