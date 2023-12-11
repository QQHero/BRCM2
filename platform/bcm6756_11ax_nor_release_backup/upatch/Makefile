include $(TOP_PATH)/rootpath.mk
include $(TOP_PATH)/targets/$(TARGETS_FILE)/config.mk
include $(TOP_PATH)/targets/$(TARGETS_FILE)/makefile.common
include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config

ifeq ($(CONFIG_TENDA_PRIVATE_KM), y)
apps_module_dir_y += km
endif

ifeq ($(CONFIG_WIFI), y)
ifeq ($(CONFIG_WIFI2), y)
else
endif
endif


ifeq ($(CONFIG_WIFI2), )
apps_module_dir_$(CONFIG_TD_MESH) += mesh
apps_module_dir_$(CONFIG_TD_MESH_V3) += mesh3.0
endif

all: 
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done


	make -C $(TOP_PATH)/platform/ all
clean: 
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done


	make -C $(TOP_PATH)/platform/ clean
