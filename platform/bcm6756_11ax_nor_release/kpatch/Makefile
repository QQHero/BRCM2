
####去掉 $(KPATCH_DIR)中的 work_dir.XXXXX
ifneq ($(PROJ_TOP),)
PROJ_TOP_KPATCH=`$(PROJ_TOP)/scripts/shadowdir/del_string_work.xxx.sh $(KPATCH_DIR)`
endif


patch: wireless
	if [ "#$(PROJ_TOP)" != "#" -a -e $(PROJ_TOP)/PLATFORM_TOP.flag ]; then \
		rsync -rvcm $(PROJ_TOP_KPATCH)/ $(KERNEL_DIR)/ ; \
	else \
		rsync -rvcm $(KPATCH_DIR)/ $(KERNEL_DIR)/ ; \
	fi
	find $(KERNEL_DIR)/ -name .svn -type d | xargs rm -rf

wireless:
	if [ "$(SOLUTION_NAME)" = "rtl8197fx_v3.4.11b" ]; then\
		if [ "$(VENDOR)" = RTK -a ! -e $(KERNEL_DIR)/drivers/net/wireless/rtl8192cd ];then \
			ln -s $(WIRELESS_DRIVER_PATH) $(KERNEL_DIR)/drivers/net/wireless/rtl8192cd; \
		fi;\
	fi
	if [ "$(KERNEL_PLATORM_NAME)" = "rtl8197gh_v3.4.13" ]; then\
		if [ "$(VENDOR)" = RTK -a ! -e $(KERNEL_DIR)/drivers/net/wireless/rtl8192cd ];then \
			ln -s $(WIRELESS_DRIVER_PATH) $(KERNEL_DIR)/drivers/net/wireless/rtl8192cd; \
		fi;\
	fi
	if [ "$(SOLUTION_NAME)" = "rtl8198d_mesh" ]; then\
		if [ "$(VENDOR)" = RTK -a ! -e $(KERNEL_DIR)/drivers/net/wireless/realtek/rtl8192cd ];then \
			ln -s $(WIRELESS_DRIVER_PATH) $(KERNEL_DIR)/drivers/net/wireless/realtek/rtl8192cd; \
		fi;\
	fi

.PHONY: wireless
