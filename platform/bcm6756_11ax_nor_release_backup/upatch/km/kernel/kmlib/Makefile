PWD := $(shell pwd)
ccflags-y :=  -I $(KM_PATH)/kernel/include

obj-m += kmlib.o
kmlib-objs := km_lib.o km_lib_netlink.o km_lib_json.o fragment_push.o


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
