include $(TOP_PATH)/rootpath.mk
include $(TOP_PATH)/tsfile.mk
include $(TOP_PATH)/targets/$(TARGETS_FILE)/config.mk
include $(TOP_PATH)/targets/$(TARGETS_FILE)/makefile.common

apps_module_dir_y += busybox
apps_module_dir_y += common
apps_module_dir_y += ucloud
apps_module_dir_$(CONFIG_USB_SUPPORT) += datacenter
apps_module_dir_y += httpd
apps_module_dir_y += agent
apps_module_dir_y += auto_discover
apps_module_dir_y += igmp_query
apps_module_dir_y += cfmc
apps_module_dir_y += cnsl_safe
apps_module_dir_y += ate
apps_module_dir_y += internet_detec
apps_module_dir_y += igmp_conflict_check
apps_module_dir_y += meshrole_probe 
apps_module_dir_y += mini_openssl 
apps_module_dir_y += syschk
apps_module_dir_$(CONFIG_CBRR) += cbrr

ifeq ($(CONFIG_SNMP_SERVER), y)
	apps_module_dir_y += snmpd
endif

ifeq ($(CONFIG_AUTO_SYNCHRO), y)
	apps_module_dir_y += pppoes
endif

ifeq ($(CONFIG_CWMPD), y)
apps_module_dir_y += cwmpd
endif

apps_module_dir_$(CONFIG_LED_SHARE_WITH_SYS) += single_led

apps_module_dir_y += kernel_modules

apps_module_dir_y += mesh
apps_module_dir_y += mpserver
all: 
	@for i in $(apps_module_dir_y) ; \
	    do make -C $$i $@ || exit $?; \
	done
	
clean: 
	@for i in $(apps_module_dir_y) ; \
	    do make -C $$i $@ || exit $?; \
	done


