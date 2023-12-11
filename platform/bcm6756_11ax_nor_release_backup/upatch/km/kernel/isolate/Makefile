include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config
module_dir_$(CONFIG_NETWORK_ISOLATE) += network_isolate


all: 
	for i in $(module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done
clean: 
	for i in $(module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done



