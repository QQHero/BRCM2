PWD := $(shell pwd)
ccflags-y :=  -I $(KM_PATH)/kernel/include

KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmlib/Module.symvers

obj-m += kmbase.o
kmbase-objs := km_base.o bm_common.o bm_http.o bm_https.o km_procfs.o km_debug.o km_wan.o km_algorithm.o km_hook.o km_public.o km_online_ip.o


_KM_SVN_VERSION := $(shell svn info $(TOP_PATH)/cbb/km/ |sed -n '/^Revision/{p}'|awk '{print $$2}')
_KM_SVN_PATH := $(shell svn info $(TOP_PATH)/cbb/km/ |sed -n '/^URL/{p}'|awk '{print $$2}')

_KM_BUILD_DATA := $(shell date)
EXTRA_CFLAGS = -DKM_SVN_VERSION="\"$(_KM_SVN_VERSION)\""
EXTRA_CFLAGS += -DKM_BUILD_DATA="\"$(_KM_BUILD_DATA)\""
EXTRA_CFLAGS += -DKM_SVN_PATH="\"$(_KM_SVN_PATH)\""


file:=$(filter-out %.mod.c,$(wildcard *.c)) 

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
ifneq (0,$(words $(file)))
	-rm -rf *.ko *~ core .depend .*.cmd *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif

.PHONY: modules modules_install clean
