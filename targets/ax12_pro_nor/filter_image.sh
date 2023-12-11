#!/bin/sh
IMAGE_ROMFS_PATH=$1

echo $IMAGE_ROMFS_PATH
echo "###########clean image not use file############"
rm -f $IMAGE_ROMFS_PATH/lib/modules/ip_mac_bind.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/port_filter.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/ufsd.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/GPL_NetUSB.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/NetUSB.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/libntfs-3g.so.81 > /dev/null
#rm $IMAGE_ROMFS_PATH/lib/libcrypto.so.1.0.0 > /dev/null
#rm $IMAGE_ROMFS_PATH/lib/libcrypto.so > /dev/null

rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/bcmdrivers/broadcom/char/pwrmngt/bcm96756/pwrmngtd.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/bcmdrivers/broadcom/net/wl/bcm96756/main/components/router/hnd/hnd.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/bcmdrivers/broadcom/net/wl/bcm96756/main/components/router/hnd_dhd/dhd.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/bcmdrivers/broadcom/net/wl/bcm96756/main/components/router/hnd_emf/emf.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/bcmdrivers/broadcom/net/wl/bcm96756/main/components/router/hnd_igs/igs.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/bcmdrivers/broadcom/net/wl/bcm96756/main/components/router/hnd_wl/wl.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/bcmdrivers/opensource/bus/pci/host/bcm96756/bcm_pcie_hcd.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/bcmdrivers/opensource/char/map/bcm96756/ivi.ko > /dev/null
#rm -rf $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/drivers/usb/ > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/4.19.183/kernel/net/wireless/cfg80211.ko > /dev/null

rm -f $IMAGE_ROMFS_PATH/lib/libntfs-3g.so.86 > /dev/null

rm -f $IMAGE_ROMFS_PATH/lib/modules/ai.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/fastnat.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/portal.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/loadbalance.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/kmwdog.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/os_identify.ko > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/modules/trace.ko > /dev/null

rm -f $IMAGE_ROMFS_PATH/usr/sbin/smbd > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/smbpasswd > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/usb_down.sh > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/usb_up.sh > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/Printer.sh > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/start_tunnel_6in4.sh  > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/start_tunnel_6rd.sh > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/start_tunnel_6to4.sh > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/acs_cli > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/acsd > /dev/null
rm -f $IMAGE_ROMFS_PATH/usr/sbin/wl > /dev/null

rm -f $IMAGE_ROMFS_PATH/bin/gdb > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/sha256sum > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/tcpdump > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/acsd2 > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/acs_cli2 > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/km_cmd > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/ntfs-3g > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/strace > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/nginx	> /dev/null
rm -rf $IMAGE_ROMFS_PATH/etc_ro/nginx > /dev/null

echo "flash cut success"
