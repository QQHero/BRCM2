include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config
module_dir_$(CONFIG_ARP_FENCE_CONTROL) += arp_fence
module_dir_$(CONFIG_DDOS_CONTROL) += ddos_ip
module_dir_$(CONFIG_MULTI_BROAD_FILTER_CONTROL) += multi_broad_filter

all: 
	for i in $(module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done
clean: 
	for i in $(module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done



