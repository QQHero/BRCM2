include $(TOP_PATH)/rootpath.mk
include $(TOP_PATH)/tsfile.mk
include $(TOP_PATH)/targets/$(TARGETS_FILE)/makefile.common
include $(TOP_PATH)/targets/$(TARGETS_FILE)/config.mk

VPATH = ./

LDFLAGS += $(LDFLAGS_UGW) -lcommonprod -lrt -lwifibase
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=199309L
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl63/main/src/shared/bcmwifi/include
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl63/main/src/wl/td_wifibase/
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl63/main/components/proto/include 
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl63/main/src/include 
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl63/main/src/wl/exe 
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl63/main/components/wlioctl/include 
CFLAGS += -I./

OBJS=guard.o
EXEC=mp_guard

all:$(EXEC)
	cp $(EXEC) $(ROMFS_PATH)/bin

$(EXEC):$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(CFLAGS) 

clean:
	rm -f $(OBJS) $(EXEC) 
