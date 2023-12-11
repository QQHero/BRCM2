#
#use this script to generate romfs directory and copy some files  from
#vender romfs,such as libc,busybox or some other config.
#
#!/bin/sh

disk="root root 755"

mkDev() {    
    # usage: mkDev name [bcu] major minor owner group mode
    if [ ! -e $1 ]
    then
        mknod $1 $2 $3 $4
        chown $5:$6 $1
        chmod $7 $1
    fi
}

mkDir() {
    if [ ! -d $1 ]
    then
        mkdir $1 -m $2
    fi
}

softLink(){
    if [ ! -f $2 ]
    then
        ln -sf $1 $2
    fi
}

PLATFORM_ROMFS_PATH=$TOP_PATH/platform/$SOLUTION_NAME\_release/platform_romfs
LIBS=$TOP_PATH/vendor/$KERNEL_PLATORM_NAME/lib
ETC=$TOP_PATH/vendor/$KERNEL_PLATORM_NAME/etc_ro
USR=$TOP_PATH/vendor/$KERNEL_PLATORM_NAME/usr
BIN=$TOP_PATH/vendor/$KERNEL_PLATORM_NAME/bin

TARGETS=$TOP_PATH/targets/$TARGETS_FILE
ROMFS=$TARGETS/romfs
WEBROOT=$WEB_PATH
WEBUPLIE=$SIMPLE_UPLOAD_FILE_PATH

if [ -d $LIBS ] && [ -d $ETC ] && [ -d $usr ] && [ -d $webroot ] 
then
    echo $LIBS
    echo $ETC
    echo $ROMFS
else
    echo "error target."
    exit 1
fi

mkDir $ROMFS 755

echo "enter $ROMFS"
cd $ROMFS

cp $USR $ROMFS -fr

dirs="bin sbin dev lib lib/modules etc_ro etc_ro/wlan proc cfg cfg_bak"
dirs=$dirs" mnt sys tmp var var/webroot var/etc"
dirs=$dirs" var/home var/root webroot_ro data"
for dir in $dirs
do
    mkDir $dir 755
done

softLink /var/etc etc
softLink /var/home home
softLink /var/root root
softLink /var/webroot webroot
softLink /var/debug debug

#rm $LIBS/libbcm_flashutil.so
#rm $BIN/bcm_flasher
cp $LIBS/* $ROMFS/lib -fr
cp $ETC/* $ROMFS/etc_ro -fr
cp $BIN/* $ROMFS/bin -fr

#start link cmd to busybox
softLink bin/busybox init

cd $ROMFS/bin
echo "enter $ROMFS/bin"
cmds="ash cat chmod cp cttyhack date dd df dmesg"
cmds=$cmds" echo false getopt grep hush kill ln login ls"
cmds=$cmds" mkdir mknod more mount msh mv netstat ping ping6 ps pwd"
cmds=$cmds" rm rmdir sed sh sleep su touch true umount uname usleep vi tar fdisk"
for cmd in $cmds
do
    softLink busybox $cmd
done

#enter romfs/sbin
cd $ROMFS/sbin
echo "enter $ROMFS/sbin/bin"
cmds="arp blkid depmod getty halt ifconfig init insmod"
cmds=$cmds" klogd logread lsmod makedevs modprobe pivot_root"
cmds=$cmds" poweroff reboot rmmod route slattach sulogin"
cmds=$cmds" sysctl syslogd vconfig eject ping6 mdev md5sum"
for cmd in $cmds
do
    softLink ../bin/busybox $cmd
done


cd $ROMFS/usr/bin
echo "enter $ROMFS/usr/bin"
cmds="[ [[ arping awk basename clear cryptpw cut diff dirname"
cmds=$cmds" du env expr find free hd head hexdump hostid id ipcs"
cmds=$cmds" killall killall5 less logger mesg mkfifo nslookup"
cmds=$cmds" passwd printf reset tail telnet test tftp top traceroute"
cmds=$cmds" uptime wc xargs yes wget unzip which tr sync"
for cmd in $cmds
do
    softLink ../../bin/busybox $cmd
done

cd $ROMFS/usr/
softLink     ../tmp     tmp
cd $ROMFS/usr/sbin
echo "enter $ROMFS/usr/sbin"
softLink    ../../bin/pptpctrl    pptpctrl

cmds="telnetd rdate"
for cmd in $cmds
do
    softLink ../../bin/busybox $cmd
done

#enter romfs/dev
cd $ROMFS/dev
mkDev    console    c    5    1    $disk
mkDev    ttyS0    c    4    64    $disk
mkDev    ttyS1    c    4    65    $disk
mkDev    ttyS2    c    4    66    $disk
mkDev    ttyS3    c    4    67    $disk
mkDev    ttyS4    c    4    68    $disk
mkDev    ttyAMA0    c    4    69    $disk


cd $ROMFS/lib
#softLink    libcrypto.so.1.0.0 libcrypto.so
softLink    libz.so libz.so.1
softLink    ld-uClibc.so ld-uClibc.so.0
softLink    libutil.so libutil.so.0

cd $ROMFS/usr/lib
softLink    libmnl.so.0.1.0    libmnl.so
softLink    libmnl.so.0.1.0    libmnl.so.0
softLink    libnetfilter_conntrack.so.3.4.0    libnetfilter_conntrack.so
softLink    libnetfilter_conntrack.so.3.4.0    libnetfilter_conntrack.so.3 
softLink    libnetfilter_queue.so.1.3.0    libnetfilter_queue.so
softLink    libnetfilter_queue.so.1.3.0    libnetfilter_queue.so.1
softLink    libnfnetlink.so.0.2.0    libnfnetlink.so
softLink    libnfnetlink.so.0.2.0    libnfnetlink.so.0
softLink    libstdc++.so.6.0.14    libstdc++.so.6
softLink    libip4tc.so.0.0.0    libip4tc.so
softLink    libip4tc.so.0.0.0    libip4tc.so.0
softLink    libip6tc.so.0.0.0    libip6tc.so
softLink    libip6tc.so.0.0.0    libip6tc.so.0
softLink    libiptc.so.0.0.0     libiptc.so
softLink    libiptc.so.0.0.0    libiptc.so.0
softLink    libxtables.so.7.0.0    libxtables.so.7
cp $TARGETS/default.cfg    $ROMFS/webroot_ro
cp $TARGETS/default_url.cfg $ROMFS/webroot_ro
cp $TARGETS/apmib.cfg $ROMFS/webroot_ro
cp -fr $TARGETS/policy.cfg $ROMFS/etc_ro/policy_bak.cfg
cp -fr $TARGETS/features.cfg $ROMFS/etc_ro/features.cfg
cp -fr $TARGETS/rcS $ROMFS/etc_ro/init.d/rcS
cp -fr $TARGETS/netctrl0 $ROMFS/etc_ro/init.d/netctrl0
cp -fr $TARGETS/gpio_conf $ROMFS/etc_ro/gpio_conf
cp -fr $TARGETS/proc.conf $ROMFS/etc_ro/proc.conf
cp -fr $TARGETS/eth_name_conf.txt $ROMFS/etc_ro/
cp -fr $TARGETS/eth_to_port_config.txt $ROMFS/etc_ro/eth_to_port_config.txt
cp -fr $TARGETS/mcpd.conf $ROMFS/etc_ro/
chmod +x $ROMFS/etc_ro/gpio_conf
cp -fr $TARGETS/inittab $ROMFS/etc_ro/inittab
cp -r $WEB_NOCACHE_PATH/* $ROMFS/webroot_ro/
#chmod 664  $ROMFS/webroot_ro/login.asp
#cp $WEBUPLIE/simple_upgrade.asp $ROMFS/webroot_ro/system_upgrade.asp
#chmod 664 $ROMFS/webroot_ro/system_upgrade.asp
#cp $WEBUPLIE/simple_upgrading.asp $ROMFS/webroot_ro/upgrading.asp
#chmod 664 $ROMFS/webroot_ro/upgrading.asp
#新加一些配置脚本用于平台技术项转测试
cp $TARGETS/config_script/wifi_init.sh $ROMFS/usr/sbin/
cp $TARGETS/config_script/ $ROMFS/usr/sbin/ -rf
mv $ROMFS/usr/sbin/config_script/del_if_global_addr.sh $ROMFS/usr/sbin/ -f
mv $ROMFS/usr/sbin/config_script/swconfig $ROMFS/bin/swconfig -f

#增加调试工具
cp $TARGETS/tcpdump $ROMFS/bin/
cp $TARGETS/strace $ROMFS/bin/
cp $TARGETS/brctl $ROMFS/bin/

#BCM硬件参数，knvram
#cp -fr $TARGETS/wlan/.kernel_nvram.setting $ROMFS/etc_ro/wlan/
cp -fr "$TARGETS/wlan/.kernel_nvram.setting_AX12PRO_KCT8243HE&KCT8528HE" $ROMFS/etc_ro/wlan/
cp -fr $TARGETS/udev/* $ROMFS/etc_ro/udev/

#桥接模式linkup、linkdown异步通知命令
cp $TARGETS/config_script/extend_link.sh $ROMFS/etc_ro/ -f

#use tendaupgrade compiled by platform not by develop
echo "cp $PLATFORM_ROMFS_PATH/bin/tendaupgrade $ROMFS_PATH/bin/ -f"
cp $PLATFORM_ROMFS_PATH/bin/tendaupgrade $ROMFS_PATH/bin/ -f

find $ROMFS -name "*.ko" | xargs $STRIP --strip-unneeded
find $ROMFS -name "*.so*" -type f | xargs $STRIP --strip-unneeded
find $ROMFS -name ".svn" | xargs rm -fr
$TOP_PATH/targets/$TARGETS_FILE/filter_image.sh $IMAGE_ROMFS_PATH
echo "generate romfs dirs ok"
#strip romfs files
shopt -s extglob
$STRIP $IMAGE_ROMFS_PATH/bin/!(*.sh|network|swconfig|wlaffinity|nvram)
$STRIP $IMAGE_ROMFS_PATH/usr/bin/curl
$STRIP $IMAGE_ROMFS_PATH/usr/bin/mcurl
$STRIP $IMAGE_ROMFS_PATH/usr/sbin/pppdv2
$STRIP $IMAGE_ROMFS_PATH/usr/sbin/pppoe-server
$STRIP $IMAGE_ROMFS_PATH/sbin/udevd

#删除没有用的文件
rm -f $IMAGE_ROMFS_PATH/webroot_ro/default_url.cfg > /dev/null
rm -rf $IMAGE_ROMFS_PATH/include > /dev/null
rm -f $IMAGE_ROMFS_PATH/etc_ro/policy_bak.cfg > /dev/null
rm -f $IMAGE_ROMFS_PATH/etc_ro/features.cfg > /dev/null
rm -f $IMAGE_ROMFS_PATH/lib/libwifibase.a > /dev/null
rm -f $IMAGE_ROMFS_PATH/bin/rpcapd > /dev/null