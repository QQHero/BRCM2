PWD := $(shell pwd)

obj-m += eventscenter.o
eventscenter-objs := events_center.o events_center_wifievents.o events_center_kmevents.o events_center_public.o

subdir-ccflags-y := -I $(KM_PATH)/kernel/include

_KM_SVN_VERSION := $(shell svn info $(TOP_PATH)/cbb/km/ |sed -n '/^Revision/{p}'|awk '{print $$2}')
_KM_SVN_PATH := $(shell svn info $(TOP_PATH)/cbb/km/ |sed -n '/^URL/{p}'|awk '{print $$2}')

_KM_BUILD_DATA := $(shell date)
EXTRA_CFLAGS = -DKM_SVN_VERSION="\"$(_KM_SVN_VERSION)\""
EXTRA_CFLAGS += -DKM_BUILD_DATA="\"$(_KM_BUILD_DATA)\""
EXTRA_CFLAGS += -DKM_SVN_PATH="\"$(_KM_SVN_PATH)\""

file:=$(filter-out %.mod.c,$(wildcard *.c))
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symvers

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules 
clean:
ifneq (0,$(words $(file)))
	-rm -rf *.ko *~ core .depend .*.cmd *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif

.PHONY: modules modules_install clean