include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config

apps_module_dir_y += fastnat
apps_module_dir_$(CONFIG_SA_FASTPATH_L2) += fast_l2

all:
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done

clean: 
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done


