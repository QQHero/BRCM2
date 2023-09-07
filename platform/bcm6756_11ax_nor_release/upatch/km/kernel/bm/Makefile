include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config
PWD := $(shell pwd)
subdir-ccflags-y := -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS := $(KM_PATH)/kernel/kmbase/Module.symvers
DEPEND_OBJ += bm_hook.o bm_u2k_info.o
file:=$(filter-out %.mod.c,$(wildcard *.c)) 

bm-objs := $(DEPEND_OBJ)
obj-m += bm.o
apps_module_dir_y += mac_filter
apps_module_dir_y += mac_group
apps_module_dir_y += ip_mac_bind
apps_module_dir_y += privilege
apps_module_dir_y += port_filter
apps_module_dir_y += url_filter
apps_module_dir_y += dns_cache
apps_module_dir_y += app_filter
apps_module_dir_$(CONFIG_PORTAL_CONTROL) += portal
apps_module_dir_y += connect_limit
apps_module_dir_y += ip_filter

_BM_SVN_VERSION := $(shell svn info $(M)|sed -n '/^Revision/{p}'|awk '{print $$2}')
_BM_BUILD_DATA := $(shell date)
EXTRA_CFLAGS += -DBM_SVN_VERSION="\"$(_BM_SVN_VERSION)\""
EXTRA_CFLAGS += -DBM_BUILD_DATA="\"$(_BM_BUILD_DATA)\""

all:
	-rm -rf u2k_info
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules 
	$(CROSS_STRIP) -S *.ko
	for i in $(apps_module_dir_y) ; \
		do make -C $$i $@ || exit $?; \
	done
	
clean: 
	for i in $(apps_module_dir_y) ; \
		do make -C $$i $@ || exit $?; \
	done
ifneq (0,$(words $(file)))
	-rm -rf *.ko *~ core .depend .*.cmd *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif


