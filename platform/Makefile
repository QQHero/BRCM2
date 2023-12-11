include $(TOP_PATH)/rootpath.mk
include $(TOP_PATH)/targets/$(TARGETS_FILE)/config.mk
include $(TOP_PATH)/targets/$(TARGETS_FILE)/makefile.common
include $(TOP_PATH)/targets/$(TARGETS_FILE)/def.linux.config

PLATFORM_UPATCH_PATH = $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/upatch
BM_PATH = $(PLATFORM_UPATCH_PATH)/km/kernel/bm
PLATFORM_VERSION_FILE = $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/doc/platform_version.h
PLATFORM_ROMFS_PATH = $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/platform_romfs
PLATFORM_KPATCH_PATH = $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/kpatch

all:
ifeq ($(CONFIG_TENDA_PRIVATE_KM),y)
	-cp $(PLATFORM_UPATCH_PATH)/km/kernel/kmlib/kmlib.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/kmlib.ko
endif
ifeq ($(CONFIG_TENDA_PRIVATE_KM),y)
	-cp $(PLATFORM_UPATCH_PATH)/km/kernel/kmbase/kmbase.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/kmbase.ko
endif
ifeq ($(CONFIG_NOS_CONTROL),y)
	-cp $(PLATFORM_UPATCH_PATH)/km/kernel/nos/nos.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/nos.ko
endif
ifeq ($(CONFIG_NOS_CONTROL_V2),y)
	-cp $(PLATFORM_UPATCH_PATH)/km/kernel/nos_v2/nos.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/nos.ko
endif
ifeq ($(CONFIG_DNS_REDIRECT_CONTROL),y)
	cp $(PLATFORM_UPATCH_PATH)/km/kernel/dns_redirect/dnsredirect.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/dnsredirect.ko
endif
ifeq ($(CONFIG_AUTO_DISCOVER),y)
	cp $(PLATFORM_UPATCH_PATH)/km/kernel/autodiscover/autodiscover.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/autodiscover.ko
endif
ifeq ($(CONFIG_DDOS_CONTROL),y)
	cp $(PLATFORM_ROMFS_PATH)/lib/modules/ddos_ip_fence.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/ddos_ip_fence.ko
endif
ifeq ($(CONFIG_ARP_FENCE_CONTROL),y)
	cp $(PLATFORM_ROMFS_PATH)/lib/modules/arp_fence.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/arp_fence.ko
endif
ifeq ($(CONFIG_SA_FASTPATH_NAT),y)
	cp $(PLATFORM_UPATCH_PATH)/km/kernel/sa/fastnat/fastnat.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/fastnat.ko
endif
ifeq ($(CONFIG_SA_FASTPATH_L2),y)
	cp $(PLATFORM_UPATCH_PATH)/km/kernel/sa/fast_l2/fast_l2.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/fast_l2.ko
endif
ifeq ($(CONFIG_NOS_CONTROL),y)
	cp $(PLATFORM_UPATCH_PATH)/km/kernel/nos/nos.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/nos.ko
endif
ifeq ($(CONFIG_LOAD_BALANCE),y)
	cp $(PLATFORM_UPATCH_PATH)/km/kernel/loadbalance/loadbalance.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/loadbalance.ko
endif
ifeq ($(CONFIG_KM_KWDOG),y)
	cp $(PLATFORM_UPATCH_PATH)/km/kernel/kwdog/kmwdog.ko $(ROMFS_PATH)/lib/modules -f
	$(CROSS_STRIP) -S $(ROMFS_PATH)/lib/modules/kmwdog.ko
endif
ifeq ($(CONFIG_APP_IDENTIFY),y)
	cp $(PLATFORM_ROMFS_PATH)/lib/modules/ai.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_BEHAVIOR_MANAGER),y)
	-cp $(BM_PATH)/bm.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_BEHAVIOR_MANAGER),y)
	-cp $(BM_PATH)/mac_group/mac_group.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_FILTER_MAC),y)
	-cp $(BM_PATH)/mac_filter/mac_filter.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_FILTER_IPMAC),y)
	-cp $(BM_PATH)/ip_mac_bind/ip_mac_bind.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_PRIVILEGE_IP),y)
	-cp $(BM_PATH)/privilege/privilege_ip.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_FILTER_IPPORT),y)
	-cp $(BM_PATH)/port_filter/port_filter.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_FILTER_URL),y)
	-cp $(BM_PATH)/url_filter/url_filter.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_APP_IDENTIFY),y)
	-cp $(BM_PATH)/app_filter/app_filter.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_PORTAL_AUTH),y)
	-cp $(BM_PATH)/portal/portal.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_CONNECT_LIMIT),y)
	-cp $(BM_PATH)/connect_limit/connect_limit.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(CONFIG_IP_FILTER),y)
	-cp $(BM_PATH)/ip_filter/ip_filter.ko $(ROMFS_PATH)/lib/modules -f
endif 
ifeq ($(CONFIG_PPPoE_SERVER),y)
	-cp $(BM_PATH)/webauth/ip_web_auth.ko $(ROMFS_PATH)/lib/modules -f
endif
ifeq ($(PLATFORM_VERSION_FILE), $(wildcard $(PLATFORM_VERSION_FILE)))
	grep "#define" $(TOP_PATH)/svn_version.h | awk '{printf "%-35s: %s <br />\n", $$2,$$3}' > $(ROMFS_PATH)/etc_ro/versions
	grep "#define" $(PLATFORM_VERSION_FILE) | awk '{printf "%-35s: %s <br />\n", $$2,$$3}' >> $(ROMFS_PATH)/etc_ro/versions
endif

ifeq ($(CONFIG_COMM_GPIO),y)
	cp $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/platform_romfs/lib/modules/gpio.ko $(ROMFS_PATH)/lib/modules -f
endif

ifeq ($(CONFIG_PHY_CHECK),y)
	cp $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/platform_romfs/lib/modules/phy_check.ko $(ROMFS_PATH)/lib/modules -f
endif

ifeq ($(CONFIG_EVENTS_CENTER),y)
	cp $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/platform_romfs/lib/modules/eventscenter.ko $(ROMFS_PATH)/lib/modules -f
endif

clean:
	rm -rf $(ROMFS_PATH)/bin/arpgateway
	rm -rf $(ROMFS_PATH)/bin/check_internet
	
	rm -rf $(ROMFS_PATH)/lib/modules/pptpc.ko
	rm -rf $(ROMFS_PATH)/lib/modules/ethqos.ko
	rm -rf $(ROMFS_PATH)/lib/modules/fastnat.ko
	rm -rf $(ROMFS_PATH)/lib/modules/nos.ko
	
	rm -rf $(ROMFS_PATH)/lib/modules/gpio.ko
	rm -rf $(ROMFS_PATH)/lib/modules/phy_check.ko
	rm -rf $(ROMFS_PATH)/lib/modules/ddos_ip_fence.ko
	
	rm -rf $(ROMFS_PATH)/lib/modules/arp_fence.ko
	rm -rf $(ROMFS_PATH)/lib/modules/bm.ko
	rm -rf $(ROMFS_PATH)/lib/modules/ai.ko
	rm -rf $(ROMFS_PATH)/lib/modules/mac_filter.ko
	rm -rf $(ROMFS_PATH)/lib/modules/ip_mac_bind.ko
	rm -rf $(ROMFS_PATH)/lib/modules/privilege_ip.ko
	rm -rf $(ROMFS_PATH)/lib/modules/port_filter.ko
	rm -rf $(ROMFS_PATH)/lib/modules/url_filter.ko
	rm -rf $(ROMFS_PATH)/lib/modules/app_filter.ko
	rm -rf $(ROMFS_PATH)/lib/modules/qos.ko
	rm -rf $(ROMFS_PATH)/lib/modules/portal.ko
	rm -rf $(ROMFS_PATH)/lib/modules/connect_limit.ko
	rm -rf $(ROMFS_PATH)/lib/modules/ip_filter.ko
	rm -rf $(ROMFS_PATH)/lib/modules/ip_web_auth.ko
	rm -rf $(ROMFS_PATH)/lib/modules/kmwdog.ko
	rm -rf $(ROMFS_PATH)/etc_ro/versions
	
