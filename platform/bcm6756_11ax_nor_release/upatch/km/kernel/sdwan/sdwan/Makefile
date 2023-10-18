PWD := $(shell pwd)

include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config
subdir-ccflags-y := -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS := $(KM_PATH)/kernel/kmbase/Module.symvers

TARGET         = sdwan.ko

TARGET	:= $(strip $(TARGET))
module := $(TARGET:%.ko=%)
obj-m := $(module).o
$(module)-objs := sdw_main.o sdw_netmap.o sdw_proc.o  sdw_workqueue.o netlink.o nxjson.o

ifeq ($(debug), 1)
EXTRA_CFLAGS += -g -DDEBUG_GLOBAL --coverage
endif
EXTRA_CFLAGS += -Werror -Wall -Wunused
file:=$(filter-out %.mod.c,$(wildcard *.c))

.PHONY: all clean

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done

clean:
ifneq (0,$(words $(file)))
	-rm -rf $(TARGET) .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf $(TARGET) .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done
