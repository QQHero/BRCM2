linux:linux_only modules_install

linux_only:
	cp -r $(TOP_PATH)/platform/bcm6756_11ax_nor_release/kpatch/bcm_v5_04L_02p1_4.19/bcmdrivers/broadcom/net/wl/impl87/main/src/wl/tenda/* $(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/src/wl/
	$(MAKEARCH_KERNEL) -C $(LINUX_PATH) KERNEL_DIR=$(KERNEL_DIR) oldconfig
	$(MAKEARCH_KERNEL) -C $(LINUX_PATH) KERNEL_DIR=$(KERNEL_DIR) -j1
	$(MAKEARCH_KERNEL) -C $(KERNEL_DIR)/build -f Bcmkernel.mk boot=$(KERNEL_DIR)  dtbs
	cp $(KERNEL_DIR)/dts/6756/96756.dtb $(TOP_PATH)/targets/$(TARGETS_FILE)/obj/binaries/linux
	cp $(KERNEL_DIR)/dts/6756/96756REF1_SG.dtb $(TOP_PATH)/targets/$(TARGETS_FILE)/obj/binaries/linux
	
modules_install:
	$(MAKEARCH_KERNEL) -C $(LINUX_PATH) TENDA_EXTRA_CFLAGS='$(EXTRA_CFLAGS)' modules -j1;
	$(MAKEARCH_KERNEL) -C $(LINUX_PATH) modules_install DEPMOD=/bin/true INSTALL_MOD_PATH=$(ROMFS_PATH);
	cp $(LINUX_PATH)/vmlinux $(IMAGE_DIR)

image_tools:
	@chmod +x $(TOOLS_PATH)/*;

image: wifiko_install image_upgrade image_flash
wifiko_install:
	cp $(KERNEL_DIR)/bcmdrivers/broadcom/char/wlcsm_ext/impl1/wlcsm.ko $(IMAGE_ROMFS_PATH)/lib/modules
	cp $(KERNEL_DIR)/bcmdrivers/opensource/bus/pci/host/impl1/bcm_pcie_hcd.ko $(IMAGE_ROMFS_PATH)/lib/modules
	cp $(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/components/router/hnd/hnd.ko $(IMAGE_ROMFS_PATH)/lib/modules
	cp $(KERNEL_DIR)/net/wireless/cfg80211.ko $(IMAGE_ROMFS_PATH)/lib/modules
	cp $(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/components/router/hnd_emf/emf.ko $(IMAGE_ROMFS_PATH)/lib/modules
	cp $(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/components/router/hnd_igs/igs.ko $(IMAGE_ROMFS_PATH)/lib/modules
	cp $(KERNEL_DIR)/bcmdrivers/broadcom/net/wl/impl87/main/components/router/hnd_wl/wl.ko $(IMAGE_ROMFS_PATH)/lib/modules
	-rm $(ROMFS_PATH)/lib/modules/4.19.183/kernel/bcmdrivers/broadcom/net/wl/bcm96756/main/components/router/hnd_wl/wl.ko

image_upgrade:
	$(TOP_PATH)/targets/$(TARGETS_FILE)/genromfs.sh
	rm -rf $(IMAGE_DIR)/FS*
	rm -rf $(IMAGE_DIR)/US*
	$(STRIP) --remove-section=.note --remove-section=.comment $(IMAGE_DIR)/vmlinux
	$(OBJCOPY) -O binary $(IMAGE_DIR)/vmlinux $(IMAGE_DIR)/vmlinux.bin
	$(TOOLS_PATH)/lzma e $(IMAGE_DIR)/vmlinux.bin $(IMAGE_DIR)/vmlinux.lz
	mv $(IMAGE_DIR)/vmlinux.lz $(IMAGE_DIR)/vmlinux.bin.lzma

	cp $(IMAGE_DIR)/vmlinux* $(IMAGE_DIR)/../obj/uboot/
	cp $(IMAGE_DIR)/vmlinux* $(IMAGE_DIR)/../obj/binaries/linux/
	$(TOOLS_PATH)/generate_linux_its --dir=$(TOP_PATH)/targets/$(TARGETS_FILE)/obj/binaries/linux --chip=6756 --kernel_compression=lzma --arch=arm --armtf	> $(TOP_PATH)/targets/$(TARGETS_FILE)/obj/binaries/brcm_full_linux.its
	$(TOOLS_PATH)/mkimage -f $(IMAGE_DIR)/../obj/binaries/brcm_full_linux.its -E $(IMAGE_DIR)/../obj/binaries/tmp_fit.itb
	fitpad2len=`$(TOOLS_PATH)/fit_header_tool --hex --pad 1280	$(IMAGE_DIR)/../obj/binaries/tmp_fit.itb` ; \
		echo $$fitpad2len; \
		$(TOOLS_PATH)/mkimage -p $$fitpad2len -f $(IMAGE_DIR)/../obj/binaries/brcm_full_linux.its -E $(IMAGE_DIR)/../obj/binaries/brcm_full_linux.itb
	
	$(TOOLS_PATH)/mksquashfsNEW $(IMAGE_ROMFS_PATH) $(IMAGE_DIR)/rootfs.squashfs -noappend -all-root -comp xz

	cp $(IMAGE_DIR)/../obj/binaries/brcm_full_linux.itb $(IMAGE_DIR)/
	dd if=/dev/zero of=$(IMAGE_DIR)/image-tmp.bin bs=2883584 count=1
	dd conv=notrunc if=$(IMAGE_DIR)/brcm_full_linux.itb of=$(IMAGE_DIR)/image-tmp.bin
	cat $(IMAGE_DIR)/rootfs.squashfs >> $(IMAGE_DIR)/image-tmp.bin

	@echo "-----------create tenda image----------------"
	$(TOOLS_PATH)/mk-kf-image -P $(CONFIG_FW_ID) -A arm -O linux -T kernel -C lzma -a 80000000 -e $(shell readelf -h $(IMAGE_DIR)/vmlinux | awk '/Entry/{print $$4}') -n "Tenda_upgrade" -d $(IMAGE_DIR)/image-tmp.bin $(IMAGE_DIR)/US_$(CONFIG_PRODUCT)V1.0br_$(FW_VERSION)_$(CONFIG_WEB_LANG)_$(SVN_VERSION)_TDC01.bin

	@$(TARGETS_DIR)/openssl enc -aes128 -d -pass file:$(TARGETS_DIR)/openssl_key/password -in $(TARGETS_DIR)/openssl_key/encrypted_pass -out $(TARGETS_DIR)/openssl_key/pass_phrase
	@$(TARGETS_DIR)/openssl rsautl -verify -pubin -inkey $(TARGETS_DIR)/openssl_key/rsa_public_key.pem -in $(TARGETS_DIR)/openssl_key/signed_key -out $(TARGETS_DIR)/openssl_key/aes_key
	@$(TARGETS_DIR)/openssl dgst -sha512 -passin file:$(TARGETS_DIR)/openssl_key/pass_phrase -sign $(TARGETS_DIR)/openssl_key/rsa_private_key.pem -out $(TARGETS_DIR)/openssl_key/signed_digest1 $(IMAGE_DIR)/US_$(CONFIG_PRODUCT)V1.0br_$(FW_VERSION)_$(CONFIG_WEB_LANG)_$(SVN_VERSION)_TDC01.bin
	@cat $(TARGETS_DIR)/openssl_key/signed_digest1 $(IMAGE_DIR)/US_$(CONFIG_PRODUCT)V1.0br_$(FW_VERSION)_$(CONFIG_WEB_LANG)_$(SVN_VERSION)_TDC01.bin > $(TARGETS_DIR)/openssl_key/signed_firmware
	@$(TARGETS_DIR)/openssl enc -aes128 -in $(TARGETS_DIR)/openssl_key/signed_firmware -out $(TARGETS_DIR)/openssl_key/encrypted_signed_firmware -pass file:$(TARGETS_DIR)/openssl_key/aes_key
	@$(TARGETS_DIR)/openssl dgst -sha512 -passin file:$(TARGETS_DIR)/openssl_key/pass_phrase -sign $(TARGETS_DIR)/openssl_key/rsa_private_key.pem -out $(TARGETS_DIR)/openssl_key/signed_digest2 $(TARGETS_DIR)/openssl_key/encrypted_signed_firmware
	@cat $(TARGETS_DIR)/openssl_key/signed_digest2 $(TARGETS_DIR)/openssl_key/encrypted_signed_firmware > $(TARGETS_DIR)/openssl_key/signed_encrypted_signed_firmware
	@du -b $(TARGETS_DIR)/openssl_key/signed_encrypted_signed_firmware | awk '{printf("%08X\n", $$0)}' | xxd -r -ps > $(TARGETS_DIR)/openssl_key/s_e_s_firmware_len
	@cat $(TARGETS_DIR)/openssl_key/s_e_s_firmware_len $(TARGETS_DIR)/openssl_key/signed_encrypted_signed_firmware > $(TARGETS_DIR)/openssl_key/upgrade_package
	@mv $(TARGETS_DIR)/openssl_key/upgrade_package $(IMAGE_DIR)/US_$(CONFIG_PRODUCT)V1.0br_$(FW_VERSION)_$(CONFIG_WEB_LANG)_$(SVN_VERSION)_TDC01.bin

	@-rm $(TARGETS_DIR)/openssl_key/signed_encrypted_signed_firmware
	@-rm $(TARGETS_DIR)/openssl_key/encrypted_signed_firmware
	@-rm $(TARGETS_DIR)/openssl_key/aes_key
	@-rm $(TARGETS_DIR)/openssl_key/pass_phrase
	@-rm $(TARGETS_DIR)/openssl_key/s_e_s_firmware_len
	@-rm $(TARGETS_DIR)/openssl_key/signed_digest1
	@-rm $(TARGETS_DIR)/openssl_key/signed_digest2
	@-rm $(TARGETS_DIR)/openssl_key/signed_firmware

image_flash:
	@echo "-----------------Create flashable software image---------------------"
	@dd conv=sync of=$(IMAGE_DIR)/flash_image.bin  if=/dev/zero bs=1024k count=16
	#bootloader
	@dd conv=notrunc of=$(IMAGE_DIR)/flash_image.bin  if=$(TARGETS_DIR)/cfez_boot.bin
	#KERNELFS1
	@dd conv=notrunc if=$(IMAGE_DIR)/image-tmp.bin of=$(IMAGE_DIR)/flash_image.bin bs=720896 seek=1
	mv $(IMAGE_DIR)/flash_image.bin $(IMAGE_DIR)/FS_$(CONFIG_PRODUCT)V1.0br_$(FW_VERSION)_$(CONFIG_WEB_LANG)_$(SVN_VERSION)_TDC01.bin
	
#	rm $(IMAGE_DIR)/image-tmp.bin
image_simple:
	@rm -fr bin/zImage*;
	@chmod +x $(TOP_PATH)/tools/*;
#	cp -fr $(TOP_PATH)/targets/$(TARGETS_FILE)/default.cfg bin/webroot
	$(OBJCOPY) -O binary -R .note -R .comment -S $(LINUX_PATH)/vmlinux $(TOP_PATH)/bin/zImage;
	$(TOP_PATH)/tools/lzma -9 -f -S .lzma $(TOP_PATH)/bin/zImage;

linux_menuconfig:
	$(MAKEARCH_KERNEL) -C $(LINUX_PATH) menuconfig


linux_clean:
	rm -rf $(LINUX_PATH)/usr/initramfs_data.cpio.gz
	rm -rf $(LINUX_PATH)/usr/initramfs_data.o
	$(MAKEARCH_KERNEL) CONFIG_WL_CONF=wlconfig_lx_router_apsta -C $(LINUX_PATH) KERNEL_DIR=$(KERNEL_DIR) clean;


