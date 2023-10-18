
PWD := $(shell pwd)
subdir-ccflags-y := -I $(KM_PATH)/kernel/include

KBUILD_EXTRA_SYMBOLS = $(KM_PATH)/kernel/bm/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symver

DEPEND_OBJ += 	bm_ip_mac_bind.o \

ip_mac_bind-objs := $(DEPEND_OBJ)
obj-m += ip_mac_bind.o
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
