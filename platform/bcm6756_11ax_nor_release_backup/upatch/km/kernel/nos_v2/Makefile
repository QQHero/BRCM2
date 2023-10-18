_NOS_SVN_VERSION := $(shell svn info $(M)|sed -n '/^Revision/{p}'|awk '{print $$2}')
_NOS_BUILD_DATA := $(shell date)
EXTRA_CFLAGS = -DNOS_SVN_VERSION="\"$(_NOS_SVN_VERSION)\""
EXTRA_CFLAGS += -DNOS_BUILD_DATA="\"$(_NOS_BUILD_DATA)\""

PWD := $(shell pwd)

subdir-ccflags-y := -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS := $(KM_PATH)/kernel/kmbase/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/bm/Module.symvers

TARGET         = nos.ko

TARGET	:= $(strip $(TARGET))
module := $(TARGET:%.ko=%)
obj-m := $(module).o
$(module)-objs := nxjson.o nos_queue.o nos_debug.o nos_track.o nos_config.o nos_user.o


ifeq ($(debug), 1)
EXTRA_CFLAGS += -g -DDEBUG_GLOBAL --coverage
endif
file:=$(filter-out %.mod.c,$(wildcard *.c))


.PHONY: all clean


all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
ifneq (0,$(words $(file)))
	-rm -rf $(TARGET) .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf $(TARGET) .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif
