
PWD := $(shell pwd)
subdir-ccflags-y := -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symvers

obj-m := bridgeinhibit.o
bridgeinhibit-objs := bridge_inhibit.o

file:=$(filter-out %.mod.c,$(wildcard *.c)) 

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
	
clean:
ifneq (0,$(words $(file)))
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif
