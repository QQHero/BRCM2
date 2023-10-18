include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config

apps_module_dir_y += sdwan
apps_module_dir_$(CONFIG_KM_SDW_FAST) += sdw_fast

all:
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done

clean: 
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done