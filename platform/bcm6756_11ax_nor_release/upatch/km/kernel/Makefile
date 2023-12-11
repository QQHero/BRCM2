include $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config
#按照依赖修改顺序，否则会出现依赖编译不通过的问题
apps_module_dir_$(CONFIG_TENDA_PRIVATE_KM) = kmlib
apps_module_dir_$(CONFIG_TENDA_PRIVATE_KM) += kmbase

ifeq ($(CONFIG_BRIDGE_BEHAVIOR_MANAGER), y)
apps_module_dir_y += bridge_bm
else ifeq ($(CONFIG_BEHAVIOR_MANAGER), y)
apps_module_dir_y += bm
endif

# 取消ai模块的编译
# apps_module_dir_$(CONFIG_TENDA_PRIVATE_KM) += ai
apps_module_dir_$(CONFIG_SPEED_TEST) += speedtest
apps_module_dir_$(CONFIG_KM_MESH) += mesh

apps_module_dir_$(CONFIG_SECURE_MODULE) += secure
apps_module_dir_$(CONFIG_SA_FASTPATH) += sa
apps_module_dir_$(CONFIG_BRIDGE_INHIBIT) += bridge_inhibit
apps_module_dir_$(CONFIG_NOS_CONTROL_V2) += nos_v2
apps_module_dir_$(CONFIG_FLOW_IDENTIFY) += flow_identify
apps_module_dir_$(CONFIG_LOAD_BALANCE) += loadbalance
apps_module_dir_$(CONFIG_DNS_REDIRECT_CONTROL) += dns_redirect
apps_module_dir_$(CONFIG_INTERFACE_ISOLATE) += interface_isolate
apps_module_dir_$(CONFIG_OS_IDENTIFY) += os_identify
apps_module_dir_$(CONFIG_DHCP_OPTIONS) += dhcp_options
apps_module_dir_$(CONFIG_TENDA_PRIVATE_KM) += kwdog
apps_module_dir_$(CONFIG_AUTO_DISCOVER) += autodiscover
apps_module_dir_$(CONFIG_ISOLATE) += isolate
apps_module_dir_$(CONFIG_KM_NF_CONNTRACK) += l2_nf_conntrack
apps_module_dir_$(CONFIG_TRACE_SKB_RUNNING_TIME) += trace
apps_module_dir_$(CONFIG_PPPOE_AUTH) += pppoe_auth
apps_module_dir_$(CONFIG_SUPER_USER) += super_user
apps_module_dir_$(CONFIG_HTTP_REDIRECT) += http_redirect
apps_module_dir_$(CONFIG_KM_SDWAN) += sdwan
apps_module_dir_$(CONFIG_EVENTS_CENTER) += events_center
apps_module_dir_$(CONFIG_NETWORK_TOPOLOGY) += network_topology
apps_module_dir_$(CONFIG_TUNNEL_FORWARD) += tunnel_forward

apps_module_dir_$(CONFIG_KM_DPI) += km_dpi
apps_module_dir_$(CONFIG_KM_AUDIT) += km_audit

all:
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done

clean: 
	for i in $(apps_module_dir_y) ; \
	do make -C $$i $@ || exit $?; \
	done


