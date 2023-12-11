
.EXPORT_ALL_VARIABLES:

TOP_PATH := $(shell pwd)

#export HOST_NCPU=2
exist_tsfile = $(shell if [ -f $(TOP_PATH)/tsfile.mk ]; then echo "exist"; else echo "notexist"; fi;)

ifdef tsfile
TARGETS_FILE := $(tsfile)
else
ifeq ($(exist_tsfile), exist)
include tsfile.mk
endif
endif

ifneq ($(TARGETS_FILE), )
TARGETS_DIR := $(TOP_PATH)/targets/$(TARGETS_FILE)
include $(TOP_PATH)/targets/$(TARGETS_FILE)/config.mk
include $(TOP_PATH)/targets/$(TARGETS_FILE)/makefile.common
endif
exist_fun = $(shell if [ -f $(TOP_PATH)/targets/$(TARGETS_FILE)/default_en.cfg ]; then echo "exist"; else echo "notexist"; fi;)

ifeq ($(CONFIG_WEB_LANG), cn)
DEFAULT_CFG_FILENAME = default.cfg
else
ifeq ($(exist_fun), exist)
DEFAULT_CFG_FILENAME = default_en.cfg
else
DEFAULT_CFG_FILENAME = default.cfg
endif
endif
KCONFIG_PATCH_PATH=$(TOP_PATH)/platform/$(SOLUTION_NAME)_release/kpatch
TARGETS_COMMON=$(TOP_PATH)/targets/common

ifeq ($(KERNEL_PLATORM_NAME), )
KERNEL_PLATORM_NAME=$(SOLUTION_NAME)
endif

FW_VERSION := `awk '/\Wdefine/{split($$0,str,"\"");print str[2];exit;}'  $(TOP_PATH)/targets/$(TARGETS_FILE)/product_version.h`
HARDWARE_VERSION := `awk '/\Wdefine/{split($$0,str,"\"");print str[2];exit;}'  $(TOP_PATH)/targets/$(TARGETS_FILE)/hardware_version.h`
SVN_ROOTURL := http://172.16.30.79:18088/svn/UGWV6.0_T/SourceCodes/Trunk
SVN_USERPASS := --username Panguoyu --password JHSD_7U8f

FIND_DIR_SVN = $(shell find . -maxdepth 1 -name ".svn")

ifeq ($(FIND_DIR_SVN), )
SVN_VERSION := $(shell git rev-parse --short HEAD)
else
SVN_VERSION  := $(shell svn info|grep Revision|tr -cd "0-9")
endif

ifeq ($(SVN_VERSION), )
SVN_VERSION := $(shell cat $(TOP_PATH)/.svn/entries |sed -n '4p')
endif
PLATFORM_SVN_VERSION := `awk '/\Wdefine/{split($$0,str,"\"");print str[2];exit;}'  $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/doc/svn_version.h`
WEB_SVN_VERSION := `awk '/\Wdefine/{split($$0,str,"\"");print str[2];exit;}'  $(WEB_PATH)/svn_version.h`
POLICY_VERSION := `awk '/ver/{split($$0,str,";");print str[2];exit;}'  $(TOP_PATH)/targets/$(TARGETS_FILE)/policy.cfg`
DEFAULT_CFG := `awk '{if (substr($$0,0,2) != "\#\#") print $0 }'  $(TOP_PATH)/targets/$(TARGETS_FILE)/$(DEFAULT_CFG_FILENAME)`
MACRO_PATH = $(MACRO_CONFIG_PATH)/macro_config.js
KM_PATH = $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/upatch/km
UPDATE_RANDOM := $(shell cat /dev/urandom|sed 's/[^a-zA-Z0-9]//g'|strings -n 6|head -n 1)

HOST_NCPU := $(shell cat /proc/cpuinfo | grep processor | grep -v grep | wc -l)
ifeq ($(HOST_NCPU),)
	HOST_NCPU := 1
endif
export HOST_NCPU

all: product
product:init check_config linux users image
	echo "########product ok#########"
init:prepare init_vendor init_chaintools init_romfs init_platform init_linux init_patch
ifeq ($(FIND_DIR_SVN), )
	@git config --replace core.filemode false
endif
	@echo "........init ok........"

prepare:
	@echo "SVN_VERSION="$(SVN_VERSION)
	
	@echo "TOP_PATH = $(shell pwd)" > $(TOP_PATH)/rootpath.mk; 
	@if [ ! -e $(TOP_PATH)/targets/$(TARGETS_FILE) ]; then \
		echo $(TOP_PATH)/"targets/"$(TARGETS_FILE)"  file error!";  \
	else \
		echo "TARGETS_FILE=$(TARGETS_FILE)" > $(TOP_PATH)/tsfile.mk ; \
	fi
	@if [ ! -e $(TOP_PATH)/targets/$(TARGETS_FILE)/bin ]; then \
		mkdir $(TOP_PATH)/targets/$(TARGETS_FILE)/bin;  \
	else \
		find $(TOP_PATH)/targets/$(TARGETS_FILE)/bin/* | grep -v cfez_boot.bin | xargs rm -rf; \
	fi
	@rm -f $(TOP_PATH)/fw_version.h
	@ln -s $(TOP_PATH)/targets/$(TARGETS_FILE)/product_version.h  $(TOP_PATH)/fw_version.h 
	@rm -f $(TOP_PATH)/hardware_version.h
	@ln -s $(TOP_PATH)/targets/$(TARGETS_FILE)/hardware_version.h  hardware_version.h
init_vendor:
	@if [ ! -e $(TOP_PATH)/vendor/$(KERNEL_PLATORM_NAME) ]; then \
		svn co $(SVN_ROOTURL)/vendor/$(KERNEL_PLATORM_NAME) $(TOP_PATH)/vendor/$(KERNEL_PLATORM_NAME) $(SVN_USERPASS);\
	fi

rootpath:
	@echo "TOP_PATH = $(shell pwd)" > $(TOP_PATH)/rootpath.mk; 


init_chaintools:
	@if [ ! -f $(CROSS_COMPILE)gcc ]; then \
		echo "unzip cross tools file ..." ; \
		(tar xvfj $(TOP_PATH)/vendor/$(KERNEL_PLATORM_NAME)/$(CROSS_TOOLS).bz2 -C / > /dev/null || true) ; \
		cp -fr $(TOP_PATH)/vendor/$(KERNEL_PLATORM_NAME)/chaintools_patch/* $(CONFIG_CROSS_COMPILER_PATH) ; \
		touch .chaintools_install; \
	fi
	@echo "init chaintools ok...";

init_romfs:
	@[ -d $(ROMFS_PATH) ] || mkdir  $(ROMFS_PATH)
	@[ -d $(ROMFS_PATH)/bin ] || mkdir  $(ROMFS_PATH)/bin
	@[ -d $(ROMFS_PATH)/sbin ] || mkdir $(ROMFS_PATH)/sbin
	@[ -d $(ROMFS_PATH)/lib ] || mkdir $(ROMFS_PATH)/lib
	@[ -d $(ROMFS_PATH)/lib/modules ] || mkdir $(ROMFS_PATH)/lib/modules
	@[ -d $(ROMFS_PATH)/usr/bin ] || mkdir -p $(ROMFS_PATH)/usr/bin
	@[ -d $(ROMFS_PATH)/usr/sbin ] || mkdir $(ROMFS_PATH)/usr/sbin
	@[ -d $(ROMFS_PATH)/usr/lib ] || mkdir $(ROMFS_PATH)/usr/lib
	@[ -d $(ROMFS_PATH)/usr/local ] || mkdir $(ROMFS_PATH)/usr/local
	@[ -d $(ROMFS_PATH)/etc_ro ] || mkdir $(ROMFS_PATH)/etc_ro
	@[ -d $(ROMFS_PATH)/etc_ro/ppp ] || mkdir $(ROMFS_PATH)/etc_ro/ppp
	@[ -d $(ROMFS_PATH)/etc_ro/init.d ] || mkdir $(ROMFS_PATH)/etc_ro/init.d

ifneq ($(findstring $(CONFIG_CHIP_VENDER), qualcomm broadcom),)	
init_wifi_release:
	@rm -rf $(TOP_PATH)/platform/$(WIFI_RELEASE_NAME)
	@if [ -e $(TOP_PATH)/platform/$(WIFI_RELEASE_NAME).bz2 ]; then \
		echo "Uncompress $(TOP_PATH)/platform/$(WIFI_RELEASE_NAME).bz2";	\
		(tar xvfj $(TOP_PATH)/platform/$(WIFI_RELEASE_NAME).bz2 -C $(TOP_PATH)/platform/ > /dev/null || true); \
	else \
		echo "No platform release package found in $(TOP_PATH)/platform/$(WIFI_RELEASE_NAME).";\
		exit 1; \
	fi
	rsync -rvcml $(TOP_PATH)/platform/$(WIFI_RELEASE_NAME)/ $(TOP_PATH)/targets/$(TARGETS_FILE)/romfs/

init_platform: init_wifi_release
else
init_platform:
endif
	@if [ -e $(TOP_PATH)/platform/$(SOLUTION_NAME)_release_backup ]; then \
		rm -rf $(TOP_PATH)/platform/$(SOLUTION_NAME)_release_backup;	\
	fi
	@if [ -e $(TOP_PATH)/platform/$(SOLUTION_NAME)_release ]; then \
		echo "mv old $(SOLUTION_NAME)_release to $(SOLUTION_NAME)_release_backup...";	\
		mv $(TOP_PATH)/platform/$(SOLUTION_NAME)_release $(TOP_PATH)/platform/$(SOLUTION_NAME)_release_backup; \
	fi
	@if [ -e $(TOP_PATH)/platform/$(SOLUTION_NAME)_release.bz2 ]; then \
		(tar xvfj $(TOP_PATH)/platform/$(SOLUTION_NAME)_release.bz2 -C $(TOP_PATH)/platform/ > /dev/null || true); \
	else \
		echo "No platform release package found in $(TOP_PATH)/platform/$(SOLUTION_NAME)_release.";\
		exit 1; \
	fi
	chmod +x $(TOP_PATH)/platform.sh;
	$(TOP_PATH)/platform.sh;
	@echo "init platform ok...";

init_linux:
	@if [ -e $(TOP_PATH)/bsp/kernel/kpatch ]; then \
		rm -rf $(TOP_PATH)/bsp/kernel/kpatch; \
	fi
	@if [ ! -e $(TOP_PATH)/bsp/kernel ]; then \
		mkdir -p $(TOP_PATH)/bsp/kernel; \
	fi
	mkdir -p $(TOP_PATH)/bsp/kernel/kpatch;
	(cp -rf $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/kpatch $(TOP_PATH)/bsp/kernel);
	@if [ ! -e $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG) ]; then \
		echo "unzip kernel file ..." ; \
		(tar xvfj $(TOP_PATH)/vendor/$(KERNEL_PLATORM_NAME)/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG).bz2 -C $(TOP_PATH)/bsp/kernel/ > /dev/null || true) ; \
	fi
	@echo "........init linux ok........"

ifneq ($(TARGETS_FILE), )
include $(TOP_PATH)/targets/$(TARGETS_FILE)/image.mk
endif

init_patch:
	cat $(TOP_PATH)/targets/$(TARGETS_FILE)/def.linux.config > $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config ; \
	chmod +x $(TOP_PATH)/bsp/kernel/kpatch/ . -R ; \
	make -C $(TOP_PATH)/bsp/kernel/kpatch/ patch ; \
	make -C $(TOP_PATH)/platform/$(SOLUTION_NAME)_release/upatch/km/kernel/kmpatch/ kmpatch; \
	if [ -e $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/drivers/net/wireless/rtl8192cd/WlanHAL/Data/ ]; then \
		cp -fr $(TOP_PATH)/targets/$(TARGETS_FILE)/WlanHal_Data/* $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/drivers/net/wireless/rtl8192cd/WlanHAL/Data/ ; \
	fi
	@if [ -d $(KCONFIG_PATCH_PATH)/bsp ]; then \
		cp $(KCONFIG_PATCH_PATH)/bsp $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/net/tenda/ -rf; \
		$(KM_PATH)/kernel/kmpatch/macro_config.sh $(KCONFIG_PATCH_PATH)/bsp/Kconfig $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config; \
	fi
	@if [ -d $(KCONFIG_PATCH_PATH)/wifi ]; then \
		cp $(KCONFIG_PATCH_PATH)/wifi $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/net/tenda/ -rf; \
		$(KM_PATH)/kernel/kmpatch/macro_config.sh $(KCONFIG_PATCH_PATH)/wifi/Kconfig $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)/.config; \
	fi
	@echo "........init patch ok........"

users: check_svn upatch_users prod_users
	@echo "........users ok........"
upatch_users:
	@make -C platform/$(SOLUTION_NAME)_release/upatch all;
prod_users:macro_config
	@make -C prod all;
menuconfig:
	@make -C config all;
menuconfig_clean:
	@make -C config clean;

init_debug:
	cp $(TOP_PATH)/targets/common/debug/* $(TOP_PATH)/targets/$(TARGETS_FILE)/romfs/bin/

check_config:
	if [ ! -f $(TARGETS_DIR)/config.mk.md5 ] ||[ "$(shell md5sum $(TARGETS_DIR)/config.mk | head -c 32)" != "$(shell cat $(TARGETS_DIR)/config.mk.md5 | head -c 32)" ];	then \
		echo "config.mk is invalid,You should "make menuconfig"  to make your macros come into effect. if you add a new one, you  need to add it to the config/config.in" ;\
		exit 1 ;\
	else \
		echo config.mk  not change ;\
	fi 

check_svn:
	@echo "#ifndef SVN_VERSION_INCLUDE_H " > svn_version.h
	@echo "#define SVN_VERSION \"$(SVN_VERSION)\"" >> svn_version.h
	@echo "#define UPDATE_RANDOM \"$(UPDATE_RANDOM)_$(SVN_VERSION)\"" >> svn_version.h
	@echo "#define PLATFORM_SVN_VERSION "\"$(PLATFORM_SVN_VERSION)\" >> svn_version.h ;
	@echo "#define WEB_SVN_VERSION "\"$(WEB_SVN_VERSION)\" >> svn_version.h ;
	@echo "#endif" >> svn_version.h
	# 备份 svn_version.h
	cp svn_version.h $(TOP_PATH)/targets/$(TARGETS_FILE)/svn_version.h
	
%_only:
	@case "$(@)" in \
	*/*) d=`expr $(@) : '\(.*\)_only'`; \
	     make -C $$d; \
	esac

%_clean:
	@case "$(@)" in \
	*/*) d=`expr $(@) : '\(.*\)_clean'`; \
	     make -C $$d clean; \
	esac

clean: linux_clean users_clean
	@echo " .... clean ...."

disk_clean:clean
	rm -fr $(TOP_PATH)/bsp/kernel/$(KERNEL_PLATORM_NAME)_$(LINUX_KERNEL_PKG)
	rm -rf $(TOP_PATH)/targets/$(TARGETS_FILE)/romfs
	rm -rf $(TOP_PATH)/platform/$(SOLUTION_NAME)_release
	find $(TOP_PATH)/targets/$(TARGETS_FILE)/bin/* | grep -v cfez_boot.bin | xargs rm -rf; 
	rm $(TOP_PATH)/fw_version.h  $(TOP_PATH)/hardware_version.h  $(TOP_PATH)/rootpath.mk $(TOP_PATH)/svn_version.h  $(TOP_PATH)/tsfile.mk
	@echo " ....disk clean ...."

users_clean:
	@make -C platform/$(SOLUTION_NAME)_release/upatch clean;
	@make -C prod clean;
prod_clean:
	@make -C prod clean;

include $(TOP_PATH)/targets/$(TARGETS_FILE)/macro_config

help: 
	@echo '"make tsfile=W20E product" to make all.'
	@echo "*********general make to make image***********"
	@echo '"make users" to make upatch,prod all.'
	@echo '"make linux" to make kernel.'
	@echo '"make image" to generate image.'
	@echo "******************************************"
	@echo '"make *_only" to make * module.'
	@echo '"make *_clean" to make clean * module.'
	@echo "*********necessary make when change**************"
	@echo '"make init_patch " you should do this when you update kpatch/'
	@echo "**********useful make**************************"
	@echo '"make tsfile=W20E init " to init config and others.'
	@echo '"make image_burn " to rebuild FS_image.bin only'
	@echo '"make svn_up " to svn up all files.'
	@echo '"make check_svn" to write svn version to file svn_version.h.'
	@echo "******************************************"
	
	
