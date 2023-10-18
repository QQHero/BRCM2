PWD := $(shell pwd)
obj-m += pppoeauth.o
pppoeauth-objs := pppoe_auth.o
file:=$(filter-out %.mod.c,$(wildcard *.c)) 
ccflags-y :=  -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symvers

ifeq ($(CONFIG_LIBC_MUSL), y)
EXTRA_CFLAGS += -DCONFIG_LIBC_MUSL
endif

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
ifneq (0,$(words $(file)))
	-rm -rf *.ko *~ core .depend .*.cmd *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif

.PHONY: modules modules_install clean

