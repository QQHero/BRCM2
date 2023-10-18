PWD := $(shell pwd)

MODULE_NAME = tunnelforward
obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs += tf_tunnel.o tf_fastpath.o

subdir-ccflags-y := -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS := $(KM_PATH)/kernel/kmbase/Module.symvers
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