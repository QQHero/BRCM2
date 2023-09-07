PWD := $(shell pwd)
subdir-ccflags-y := -I $(KM_PATH)/kernel/include

KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symver
dnsredirect-objs := dns_redirect.o
obj-m += dnsredirect.o

file:=$(filter-out %.mod.c,$(wildcard *.c))
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symvers

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR)  M=$(PWD) modules 
clean:
	#rm -rf  *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d 

ifneq (0,$(words $(file)))
	-rm -rf *.ko *~ core .depend .*.cmd *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif
