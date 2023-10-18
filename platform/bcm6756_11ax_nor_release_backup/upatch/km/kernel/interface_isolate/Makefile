PWD := $(shell pwd)
obj-m += interfaceisolate.o
interfaceisolate-objs := interface_isolate.o
file:=$(filter-out %.mod.c,$(wildcard *.c)) 

subdir-ccflags-y := -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS := $(KM_PATH)/kernel/kmbase/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/bm/Module.symvers

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
ifneq (0,$(words $(file)))
	-rm -rf *.ko *~ core .depend .*.cmd *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif

.PHONY: modules modules_install clean
