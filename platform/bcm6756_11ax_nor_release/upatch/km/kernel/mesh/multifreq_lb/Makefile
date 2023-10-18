PWD := $(shell pwd)
subdir-ccflags-y := -I $(KM_PATH)/kernel/include
KBUILD_EXTRA_SYMBOLS += $(KM_PATH)/kernel/kmbase/Module.symver

multifreq_loadbalance-objs := multifreq_lb.o multifreq_lb_link.o multifreq_lb_flow.o multifreq_lb_neighbor.o multifreq_lb_dec.o multifreq_lb_wifi.o multifreq_lb_debug.o
obj-m += multifreq_loadbalance.o

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
ifneq (0,$(words $(file)))
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif

.PHONY: modules modules_install clean

