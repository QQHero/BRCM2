
PWD := $(shell pwd)
MODULE_NAME := mesh_route

obj-m += ${MODULE_NAME}.o

ifneq ($(WIFI_INSTALL_PATH), )
INSTALL_PATH=$(WIFI_INSTALL_PATH)
else
endif

${MODULE_NAME}-y += mesh_route_main.o
${MODULE_NAME}-y += mesh_route_proc.o
${MODULE_NAME}-y += mesh_route_proxy.o
${MODULE_NAME}-y += mesh_route_mnode.o
${MODULE_NAME}-y += mesh_route_tactic.o
${MODULE_NAME}-y += mesh_route_handle.o
${MODULE_NAME}-y += mesh_route_common.o
${MODULE_NAME}-y += k_mesh_ext.o
subdir-ccflags-y := -I $(KM_PATH)/kernel/include

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
	cp ${MODULE_NAME}.ko $(INSTALL_PATH)/lib/modules

clean:
	rm -f $(INSTALL_PATH)/lib/modules/${MODULE_NAME}.ko
	rm -rf  *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d *.o

.PHONY: modules modules_install clean
