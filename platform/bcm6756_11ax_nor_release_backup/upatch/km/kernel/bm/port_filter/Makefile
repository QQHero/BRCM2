
PWD := $(shell pwd)
subdir-ccflags-y := -I $(KM_PATH)/kernel/include

KBUILD_EXTRA_SYMBOLS = $(KM_PATH)/kernel/bm/mac_group/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symvers

DEPEND_OBJ += 	bm_port_filter.o \

port_filter-objs := $(DEPEND_OBJ)
obj-m += port_filter.o
file:=$(filter-out %.mod.c,$(wildcard *.c)) 
all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules 
	$(CROSS_STRIP) -S *.ko
	
clean:
ifneq (0,$(words $(file)))
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif
