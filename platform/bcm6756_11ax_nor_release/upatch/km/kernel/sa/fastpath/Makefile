#
# Makefile for the fastpath modules on top of IPv4.
#

include $(DIR_ROOT)/.config

#EXTRA_CFLAGS += -I $(DIR_LINUX)/drivers/net/rtl819x
EXTRA_CFLAGS += -I $(DIR_LINUX)/net
EXTRA_CFLAGS += -D__KERNEL__
#EXTRA_CFLAGS += -Wno-implicit -Werror

#ifeq "$(CONFIG_MODEL_RTL89xxC_SDK)" "y"
EXTRA_CFLAGS += --save-temps
#endif

ifeq ($(CONFIG_RTL_IPTABLES_FAST_PATH),m)
EXTRA_CFLAGS += -DCONFIG_RTK_IPTABLES_FAST_PATH
EXTRA_CFLAGS += -DCONFIG_FAST_PATH_MODULE

FASTPATH_OBJ=fastpath_core.o fastpath_common.o fast_pptp_core.o fast_l2tp_core.o

FASTPATH_MODULE=fastpath.o
obj-$(CONFIG_RTL_IPTABLES_FAST_PATH) += $(FASTPATH_MODULE)
fastpath-objs := $(FASTPATH_OBJ)

else
obj-$(CONFIG_RTL_IPTABLES_FAST_PATH) += fastpath_core.o fastpath_common.o fast_pptp_core.o fast_l2tp_core.o



ifeq ($(CONFIG_RTL_FAST_PPPOE), y)
obj-$(CONFIG_RTL_IPTABLES_FAST_PATH) += fast_pppoe_core.o
endif

ifeq ($(CONFIG_RTL_FAST_IPV6), y)
obj-$(CONFIG_RTL_IPTABLES_FAST_PATH) += fast_ipv6_core.o
endif
endif
obj-$(CONFIG_FASTPATH_L2_CACHE_SUPPORT) += fast_l2_core.o
obj-$(CONFIG_RTL_FAST_BRIDGE) += fast_br.o 
file:=$(filter-out %.mod.c,$(wildcard *.c)) 

clean:
ifneq (0,$(words $(file)))
	-rm -rf *.ko *.mod.c
else
	-rm -rf *.ko *.mod.c
endif
EXTRA_AFLAGS += $(EXTRA_CFLAGS)
