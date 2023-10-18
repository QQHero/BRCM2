PWD := $(shell pwd)
subdir-ccflags-y := -I $(KM_PATH)/kernel/include
obj-m += fastnat.o
fastnat-objs := hndctf.o hndctf_public.o 

ifeq ($(CONFIG_SA_FASTPATH_IPV6), y)
fastnat-objs += hndctf_ipv6.o
endif
ifeq ($(CONFIG_SA_FASTPATH_SUPPORT_RPS_RFS), y)
fastnat-objs += hndctf_rps_rfs_adapt.o
endif

_CTF_SVN_VERSION := $(shell svn info $(M)|sed -n '/^Revision/{p}'|awk '{print $$2}')
_CTF_BUILD_DATA := $(shell date)
GCC_VERSION = $(shell echo `$(CC) -dumpversion | cut -f1-2 -d.` \>= 5.5 | sed -e 's/\./*100+/g' | bc )
EXTRA_CFLAGS = -DCTF_SVN_VERSION="\"$(_CTF_SVN_VERSION)\""
EXTRA_CFLAGS += -DCTF_BUILD_DATA="\"$(_CTF_BUILD_DATA)\""
ifeq ($(GCC_VERSION), 1)
EXTRA_CFLAGS += -Wno-date-time
endif
file:=$(filter-out %.mod.c,$(wildcard *.c))

all:
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR) M=$(PWD) modules
clean:
ifneq (0,$(words $(file)))
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
else
	-rm -rf *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.order *.symvers .tmp* .*.o.d
endif

.PHONY: modules modules_install clean
