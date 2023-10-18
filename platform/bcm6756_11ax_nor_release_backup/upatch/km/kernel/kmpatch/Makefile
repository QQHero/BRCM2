KM_PATCH_SRCPATH = $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/net/tenda/
KM_PATCH_INCPATH = $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/include/net
KERNEL_CONFIG_PATH = $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config
CUR_DIR = $(shell pwd)

##CURR_DIR_REAL 表示真实的目录 而不是work_dir.XXXXX下的影子目录
ifneq ($(PROJ_TOP),)
CURR_DIR_REAL=`$(PROJ_TOP)/scripts/shadowdir/del_string_work.xxx.sh $(CUR_DIR)`
endif

kmpatch:
	@if [ -e $(PROJ_TOP)/PLATFORM_TOP.flag -a "#$(PROJ_TOP)" != "#" ]; then \
		cp -rf $(CURR_DIR_REAL)/km $(KM_PATCH_SRCPATH) ; \
		cp $(CURR_DIR_REAL)/km_common.h $(KM_PATCH_INCPATH) ; \
		cp $(CURR_DIR_REAL)/km_lib.h $(KM_PATCH_INCPATH) ; \
		$(CUR_DIR)/macro_config.sh $(KM_PATCH_SRCPATH)/Kconfig  $(KERNEL_CONFIG_PATH) ; \
		$(CUR_DIR)/macro_config.sh $(CURR_DIR_REAL)/km/Kconfig  $(KERNEL_CONFIG_PATH) ; \
	else \
		cp -rf $(CUR_DIR)/km $(KM_PATCH_SRCPATH) ; \
		cp $(CUR_DIR)/km_common.h $(KM_PATCH_INCPATH) ; \
		cp $(CUR_DIR)/km_lib.h $(KM_PATCH_INCPATH) ; \
		$(CUR_DIR)/macro_config.sh $(KM_PATCH_SRCPATH)/Kconfig  $(KERNEL_CONFIG_PATH) ; \
		$(CUR_DIR)/macro_config.sh $(CUR_DIR)/km/Kconfig  $(KERNEL_CONFIG_PATH) ; \
	fi

