PWD := $(shell pwd)
include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config
SOURCE = $(wildcard src/*.c)

ifeq ($(SOURCE), )
all:
	echo "the environment is product and not build compile"
clean:
	echo "then environment is product and not clean"
else

EXEC = libkm.so 

CFLAGS += -I$(PWD)/include
CFLAGS +=-I$(TOP_PATH)/infra/msgd -I$(TOP_PATH)/infra/cmdmgr/include
CFLAGS += -fvisibility=hidden
# 如果使用nos_v2模块，需要打开以下宏
ifeq ($(CONFIG_NOS_CONTROL_V2), y)
	CFLAGS +=-DCONFIG_NOS_NEW
endif

OBJS = $(patsubst %.c,%.o, $(wildcard src/*.c))

all: $(EXEC) 
	echo 'compile ok.'

$(EXEC): $(OBJS)
	$(CROSS_STRIP) -S --strip-unneeded  $@

$(OBJS):%.o:%.c
	$(CC) -c $(CFLAGS) $(LDFLAGS_UGW) $< -o $@
clean:
	-rm -f $(EXEC) src/*.o *.so

endif
