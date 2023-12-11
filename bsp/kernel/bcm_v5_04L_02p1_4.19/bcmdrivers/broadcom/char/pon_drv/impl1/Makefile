#
# Makefile for the BCM Pon Drv module
#

SERDES_SDK_SRC_DIR = $(src)/serdes/eagle_onu10g_src
SERDES_SDK_INC_DIR = $(src)/serdes/eagle_onu10g_include

ifneq (,$(filter $(BRCM_FULL_CHIP_NAME),6846))
    WAN_TOP_HAL_IMPL=impl5
else ifeq ("$(BRCM_FULL_CHIP_NAME)", "6858B0")
    WAN_TOP_HAL_IMPL=impl2
else ifeq ("$(BRCM_FULL_CHIP_NAME)", "6856A0")
    WAN_TOP_HAL_IMPL=impl6
else ifneq ($(findstring 63158,$(BRCM_FULL_CHIP_NAME)),)
    WAN_TOP_HAL_IMPL=impl4
else ifeq ("$(BRCM_FULL_CHIP_NAME)", "6878")
    WAN_TOP_HAL_IMPL=impl5
else ifeq ("$(BRCM_FULL_CHIP_NAME)", "6855")
    WAN_TOP_HAL_IMPL=impl6
else
    $(error unsupported BRCM_FULL_CHIP_NAME $(BRCM_FULL_CHIP_NAME))
endif

WAN_TOP_RU_GEN_SRCS = $(notdir $(shell ls $(src)/wan_top/$(WAN_TOP_HAL_IMPL)/ru_gen/*.c))
WAN_TOP_RU_GEN_OBJS = $(addprefix wan_top/$(WAN_TOP_HAL_IMPL)/ru_gen/,$(WAN_TOP_RU_GEN_SRCS:.c=.o))

WAN_TOP_HAL_SRCS = $(notdir $(shell ls $(src)/wan_top/$(WAN_TOP_HAL_IMPL)/hal/*.c))
WAN_TOP_HAL_OBJS = $(addprefix wan_top/$(WAN_TOP_HAL_IMPL)/hal/, $(WAN_TOP_HAL_SRCS:.c=.o))

obj-$(CONFIG_BCM_PON_DRV) += bcm_pondrv.o


bcm_pondrv-objs := pon_drv.o
bcm_pondrv-objs += serdes_wrapper.o
bcm_pondrv-objs += serdes_debug.o
bcm_pondrv-objs += pon_drv_serdes_util.o
bcm_pondrv-objs += pon_drv_gpon_init.o
bcm_pondrv-objs += pon_drv_epon_init.o

bcm_pondrv-objs += $(WAN_TOP_RU_GEN_OBJS)
bcm_pondrv-objs += $(WAN_TOP_HAL_OBJS)

EXTRA_CFLAGS += -I$(src)/wan_top/$(WAN_TOP_HAL_IMPL)/hal -I$(src)/wan_top/$(WAN_TOP_HAL_IMPL)/ru_gen -I$(src)/wan_top
EXTRA_CFLAGS += -I$(SERDES_SDK_INC_DIR)
EXTRA_CFLAGS += -I$(SERDES_SDK_SRC_DIR)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)/pmc
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += $(INC_RDP_FLAGS)
EXTRA_CFLAGS += -Werror -Wfatal-errors

EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/framework -I$(INC_BDMF_PATH)/system 
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/system/linux -I$(INC_BDMF_PATH)/system/linux/cm 
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/shell
#EXTRA_CFLAGS += -DUSE_BDMF_SHELL
EXTRA_CFLAGS += -Wno-error=pointer-to-int-cast -Wno-error=int-to-pointer-cast
EXTRA_CFLAGS += -DPLL_PPM_ADJ


-include $(TOPDIR)/Rules.make

INSTALL_DONE = $(src)/.done_install
INSTALL_DST_DIR = $(INSTALL_DIR)/bin/
INSTALL_SRC = $(src)/scripts/serdesctrl

clean-files := $(notdir $(INSTALL_DONE))

$(obj)/bcm_pondrv.o: $(INSTALL_DONE)

$(INSTALL_DONE): $(INSTALL_SRC)
	$(Q)install -m 755 $^ $(INSTALL_DST_DIR)
	$(Q)touch $@

# Makefile trick: breaking the following into two rules allows
# the "%.o : %.c" rule (defined earlier by kbuild), to take precidence
%.o : %.o_tmp
	$(Q)mv $< $@

%.o_tmp : %.$(BCMARCH).o_saved
	@echo "Using $<"
	$(Q)cp $< $@

.PHONY: clean

