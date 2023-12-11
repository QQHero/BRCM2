PWD := $(shell pwd)
obj-m += loadbalance.o
loadbalance-objs := load_balance.o
subdir-ccflags-y := -I $(KM_PATH)/kernel/include

_LB_SVN_VERSION := $(shell svn info $(M)|sed -n '/^Revision/{p}'|awk '{print $$2}')
_LB_BUILD_DATA := $(shell date)

EXTRA_CFLAGS = -DLB_SVN_VERSION="\"$(_LB_SVN_VERSION)\""
EXTRA_CFLAGS += -DLB_BUILD_DATA="\"$(_LB_BUILD_DATA)\""
file:=$(filter-out %.mod.c,$(wildcard *.c)) 
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symvers

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
ifneq (0,$(words $(file)))
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif

.PHONY: modules modules_install clean
