include $(TOP_PATH)/targets/$(TARGETS_FILE)/config.mk
EXEC=event
OBJS := $(patsubst %.c,%.o,$(shell ls *.c))
CFLAGS += -g

# LDFLAGS += $(LDFLAGS_UGW) -lcommonprod -lmsgapi -lcm
LDFLAGS += -L$(ROMFS_PATH)/lib -lem_shared $(LDFLAGS_UGW) -lcommonprod -lmsgapi -lcm -lwifibase -lwshared
CFLAGS += -I$(TOP_PATH)/platform/$(SOLUTION_NAME)_release/include
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/src/shared/bcmwifi/include
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/src/wl/td_wifibase/
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/components/proto/include 
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/src/include 
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/src/wl/exe 
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/components/wlioctl/include 
CFLAGS += -I$(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/src/wl/sys
CFLAGS += -I$(KERNEL_DIR)
CFLAGS += -DCONFIG_TENDA_GAME_SPEEDUP
CFLAGS += -I./
all:$(EXEC)
	cp $(EXEC) $(ROMFS_PATH)/bin
$(EXEC): ${OBJS}
	${CC} -Wall -Werror -o $@ $(OBJS) $(LDFLAGS) $(CFLAGS)

clean:
	rm -f $(EXEC) *.o
