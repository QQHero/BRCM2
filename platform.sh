#!/bin/bash

#
#use this script to link platform_romfs directory to vendor of product directory
#add by zzh @ 2016.06.02
#

PLATFORM_ROMFS_PATH=$TOP_PATH/platform/$SOLUTION_NAME\_release/platform_romfs
PLATFORM_WIFI_ROMFS_PATH=$TOP_PATH/platform/$WIFI_RELEASE_NAME

############copy platform bin to product romfs################
#busybox改为软件合入，根据不同内存产品，自行进行裁剪
#cp $PLATFORM_ROMFS_PATH/bin/busybox $ROMFS_PATH/bin/ -fr
cp $PLATFORM_ROMFS_PATH/bin/ip $ROMFS_PATH/bin/ -fr
cp $PLATFORM_ROMFS_PATH/bin/cfmd $ROMFS_PATH/bin/ -fr
cp $PLATFORM_ROMFS_PATH/bin/network $ROMFS_PATH/bin/ -fr
cp $PLATFORM_ROMFS_PATH/bin/swconfig $ROMFS_PATH/bin/ -fr
if [ "$CONFIG_CHIP_VENDER" == "broadcom" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/envram $ROMFS_PATH/bin/ -fr
	#cp $PLATFORM_ROMFS_PATH/bin/nvram $ROMFS_PATH/bin/ -fr
	cp $PLATFORM_ROMFS_PATH/bin/ethswctl $ROMFS_PATH/bin/ -fr
	cp $PLATFORM_ROMFS_PATH/bin/vlanctl $ROMFS_PATH/bin/ -fr
	cp $PLATFORM_ROMFS_PATH/lib/libethswctl.so $ROMFS_PATH/lib/ -fr
	cp $PLATFORM_ROMFS_PATH/bin/bcmmcastctl $ROMFS_PATH/bin/ -fr
	cp $PLATFORM_ROMFS_PATH/lib/libbcmmcast.so $ROMFS_PATH/lib/ -fr
fi

cp $PLATFORM_ROMFS_PATH/bin/monitor $ROMFS_PATH/bin/ -fr
cp $PLATFORM_ROMFS_PATH/bin/pservice_tool $ROMFS_PATH/bin/ -fr
cp $PLATFORM_ROMFS_PATH/bin/logserver $ROMFS_PATH/bin/ -fr
#cp $PLATFORM_ROMFS_PATH/bin/logdebug $ROMFS_PATH/bin/ -fr
cp $PLATFORM_ROMFS_PATH/bin/vsftpd $ROMFS_PATH/bin/ -fr 
if [ "$CONFIG_EBTABLES_TOOL" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/ebtables $ROMFS_PATH/bin/ -fr 
fi

if [ "$CONFIG_NET_DHCP" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/dhcps $ROMFS_PATH/bin/ -fr
	ln -fs ./dhcps $ROMFS_PATH/bin/lan_dhcps0
	if [ "$CONFIG_WIFI_GUEST" == "y" ]
	then
		ln -fs ./dhcps $ROMFS_PATH/bin/dhcps-guest
	fi
fi
cp $PLATFORM_ROMFS_PATH/bin/msgd $ROMFS_PATH/bin/ -fr
cp $PLATFORM_ROMFS_PATH/bin/dhcpcd $ROMFS_PATH/bin/ -fr
if [ "$CONFIG_LAN_DHCPC" == "y" ]
then
	ln -fs ./dhcpcd $ROMFS_PATH/bin/dhcpcd_lan
fi

if [ "$CONFIG_WAN_NUMBER" != "0" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/chat $ROMFS_PATH/bin/ -fr
	cp $PLATFORM_ROMFS_PATH/bin/pppd $ROMFS_PATH/bin/ -fr
	# WAN PPPOE
	ln -fs ./pppd $ROMFS_PATH/bin/ppoe_wan1
	ln -fs ./pppd $ROMFS_PATH/bin/ppoe_wan2
	# WAN PPTP
	ln -fs ./pppd $ROMFS_PATH/bin/pptp_wan1
	ln -fs ./pppd $ROMFS_PATH/bin/pptp_wan2	
	# WAN DUBLE_PPPOE
	ln -fs ./pppd $ROMFS_PATH/bin/db_ppoe_wan1
	ln -fs ./pppd $ROMFS_PATH/bin/db_ppoe_wan2
	# VPN PPTP server
	ln -fs ./pppd $ROMFS_PATH/bin/pptppppd-server
	ln -fs ./pppd $ROMFS_PATH/bin/pptppppd
	if [ "$CONFIG_IPV6_SUPPORT" == "y" ]
	then
		ln -fs ./pppd $ROMFS_PATH/bin/pppd_ipv6
	fi
	cp $PLATFORM_ROMFS_PATH/bin/dnrd $ROMFS_PATH/bin/ -fr
	ln -fs ./dnrd $ROMFS_PATH/bin/dnrd-guest
	ln -fs ./dnrd $ROMFS_PATH/bin/dnrd-wisp
	
	if [ "$CONFIG_NET_WAN_DHCP" == "y" ]
	then
		# WAN DHCP
		ln -fs ./dhcpcd $ROMFS_PATH/bin/dhcp_wan1
		ln -fs ./dhcpcd $ROMFS_PATH/bin/dhcp_wan2
		# WAN PPTP_DHCP
		ln -fs ./dhcpcd $ROMFS_PATH/bin/pptp_dhcp_wan1
		ln -fs ./dhcpcd $ROMFS_PATH/bin/pptp_dhcp_wan2		
		# WAN L2TP_DHCP
		ln -fs ./dhcpcd $ROMFS_PATH/bin/l2tp_dhcp_wan1
		ln -fs ./dhcpcd $ROMFS_PATH/bin/l2tp_dhcp_wan2	
		# WAN DUBLE_PPPOE_DHCP
		ln -fs ./dhcpcd $ROMFS_PATH/bin/db_dhcp_wan1
		ln -fs ./dhcpcd $ROMFS_PATH/bin/db_dhcp_wan2
		# WISP DHCP
		ln -fs ./dhcpcd $ROMFS_PATH/bin/dhcpcd_wisp
	fi
	
	if [ "$CONFIG_NET_WAN_L2TP" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/xl2tpd-client $ROMFS_PATH/bin/ -fr
		ln -fs ./xl2tpd-client $ROMFS_PATH/bin/l2tp_wan1
		ln -fs ./xl2tpd-client $ROMFS_PATH/bin/l2tp_wan2
	fi
fi

if [ "$CONFIG_ARP_GATEWAY" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/arpgateway $ROMFS_PATH/bin -fr
fi
if [ "$CONFIG_WIFI" == "y" ]
then
	if [ "$CONFIG_CHIP_VENDER" == "broadcom" ]
	then
		#cp $PLATFORM_ROMFS_PATH/bin/eapd $ROMFS_PATH/bin/ -fr
		cp ${PLATFORM_WIFI_ROMFS_PATH}/bin/eapd ${ROMFS_PATH}/bin/ -fr
	fi
	if [ "$CONFIG_CHIP_VENDER" == "realtek" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/sysconf $ROMFS_PATH/bin/ -fr
		cp $PLATFORM_ROMFS_PATH/bin/wlapp $ROMFS_PATH/bin/ -fr 2> /dev/null
		cp $PLATFORM_ROMFS_PATH/bin/wscd $ROMFS_PATH/bin/ -fr
		cp $PLATFORM_ROMFS_PATH/bin/iwcontrol $ROMFS_PATH/bin/ -fr
		cp $PLATFORM_ROMFS_PATH/lib/libwshared.so $ROMFS_PATH/lib/ -fr
		cp $PLATFORM_ROMFS_PATH/bin/wcli $ROMFS_PATH/bin/ -fr
		cp $PLATFORM_ROMFS_PATH/bin/wserver $ROMFS_PATH/bin/ -fr
        	cp $PLATFORM_ROMFS_PATH/bin/UDPserver $ROMFS_PATH/bin/ -fr
	fi
fi

if [ "$CONFIG_ADVANCE_DDNS" == "y" ]
then
	#if [ "$CONFIG_ADVANCE_3322" == "y" ]
	#then
		#cp $PLATFORM_ROMFS_PATH/bin/3322ip $ROMFS_PATH/bin/ -fr
	#fi
	if [ "$CONFIG_ADVANCE_88IP" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/88ip $ROMFS_PATH/bin/ -fr
	fi
	if [ "$CONFIG_ADVANCE_GNWAY" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/gnway $ROMFS_PATH/bin/ -fr
	fi
	if [ "$CONFIG_ADVANCE_ORAY" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/phddns $ROMFS_PATH/bin/ -fr
		ln -fs ./phddns $ROMFS_PATH/bin/phddns_wan1
		ln -fs ./phddns $ROMFS_PATH/bin/phddns_wan2
		ln -fs ./phddns $ROMFS_PATH/bin/phddns_wan3
		ln -fs ./phddns $ROMFS_PATH/bin/phddns_wan4
	fi
	if [ "$CONFIG_ADVANCE_NOIP" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/inadyn $ROMFS_PATH/bin/ -fr
	fi
	if [ "$CONFIG_ADVANCE_DYNDNS" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/inadyn $ROMFS_PATH/bin/ -fr
	fi
fi

if [ "$CONFIG_ADVANCE_UPNP" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/miniupnpd $ROMFS_PATH/bin/ -fr
fi

if [ "$CONFIG_SYSTEM_SNTP" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/sntp $ROMFS_PATH/bin/ -fr
fi

if [ "$CONFIG_VPN" == "y" ]
then
	if [ "$CONFIG_VPN_PPTP" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/pptpd $ROMFS_PATH/bin/ -fr
		cp $PLATFORM_ROMFS_PATH/bin/pptp_callmgr $ROMFS_PATH/bin/ -fr
		cp $PLATFORM_ROMFS_PATH/bin/pppdForPppServer $ROMFS_PATH/bin/ -fr
		ln -fs ./pppdForPppServer $ROMFS_PATH/bin/pppdForPptp
	fi
	if [ "$CONFIG_VPN_L2TP" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/xl2tpd-client $ROMFS_PATH/bin/ -fr
		ln -sf ./pppd $ROMFS_PATH/bin/xl2tpcltpppd
		#cp $PLATFORM_ROMFS_PATH/bin/xl2tpd-server $ROMFS_PATH/bin/ -fr
		#cp $PLATFORM_ROMFS_PATH/bin/xl2tpdpppd $ROMFS_PATH/bin/ -fr
		#ln -fs ./pppdForPppServer $ROMFS_PATH/bin/xl2tpdpppd
	fi
	if [ "$CONFIG_VPN_IPSEC" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/racoon $ROMFS_PATH/bin/ -fr
		cp $PLATFORM_ROMFS_PATH/bin/racoonctl $ROMFS_PATH/bin/ -fr
		cp $PLATFORM_ROMFS_PATH/bin/setkey $ROMFS_PATH/bin/ -fr
		if [ "$OPEN_SSL_SUPPORT" == "y" ]
		then
			cp $PLATFORM_ROMFS_PATH/bin/openssl $ROMFS_PATH/bin/ -fr
		fi
	fi
	if [ "$CONFIG_IGMPPROXY_SUPPORT" == "y" ] 
	then
		cp $PLATFORM_ROMFS_PATH/bin/igmpproxy $ROMFS_PATH/bin/ -fr
	fi
	if [ "$CONFIG_PORT_SNOOPING" == "y" ] 
	then
		cp $PLATFORM_ROMFS_PATH/bin/portsnooping $ROMFS_PATH/bin/ -fr
	fi
	if [ "$CONFIG_FTP_SERVER" == "y" ] 
	then
		cp $PLATFORM_ROMFS_PATH/bin/stupid-ftpd $ROMFS_PATH/bin/ -fr
	fi
	cp $PLATFORM_ROMFS_PATH/bin/taskset $ROMFS_PATH/bin/ -fr
	cp $PLATFORM_ROMFS_PATH/bin/arpbrocast $ROMFS_PATH/bin/ -fr
fi

if [ "$CONFIG_AC_MANAGEMENT_V2" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/sqlite3 $ROMFS_PATH/bin -fr
fi

if [ "$CONFIG_IPV6_SUPPORT" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/dhcp6c $ROMFS_PATH/bin/ -fr
	cp $PLATFORM_ROMFS_PATH/bin/dhcp6s $ROMFS_PATH/bin/ -fr
	cp $PLATFORM_ROMFS_PATH/bin/radvd $ROMFS_PATH/bin/ -fr
fi

###########copy platform etc_ro to product romfs#################
cp $PLATFORM_ROMFS_PATH/etc_ro $ROMFS_PATH/ -fr

###########copy platform lib to product romfs####################
cp $PLATFORM_ROMFS_PATH/lib/libcommon.so $ROMFS_PATH/lib -fr
cp $PLATFORM_ROMFS_PATH/lib/libz.so $ROMFS_PATH/lib -fr
cp $PLATFORM_ROMFS_PATH/lib/libiofdrv.so $ROMFS_PATH/lib -fr
cp $PLATFORM_ROMFS_PATH/lib/libmsgapi.so $ROMFS_PATH/lib -fr

if [ "$CONFIG_CHIP_VENDER" == "realtek" ]
then
	cp $PLATFORM_ROMFS_PATH/lib/libapmib.so $ROMFS_PATH/lib -fr
	cp $PLATFORM_ROMFS_PATH/bin/flash $ROMFS_PATH/bin/ -fr
	if [ "$CONFIG_WIFI" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/lib/librtlWifiSrc.so $ROMFS_PATH/lib -fr
	fi
	if [ "$SWITCH_NONE" != "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/swconfig $ROMFS_PATH/bin/ -fr
	fi
fi
if [ "$CONFIG_AC_MANAGEMENT_V2" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/lib/libsqlite3.so.0.8.6 $ROMFS_PATH/lib -fr
	ln -fs ./libsqlite3.so.0.8.6 $ROMFS_PATH/lib/libsqlite3.so
	ln -fs ./libsqlite3.so.0.8.6 $ROMFS_PATH/lib/libsqlite3.so.0
fi
if [ "$CONFIG_NET_WAN_PPPOE" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/lib/rp-pppoe.so $ROMFS_PATH/lib -fr
fi
cp $PLATFORM_ROMFS_PATH/lib/pppol2tp.so $ROMFS_PATH/lib -fr 
if [ "$CONFIG_WIFI" == "y" ]
then
	if [ "$CONFIG_CHIP_VENDER" == "broadcom" ]
	then
		cp $PLATFORM_ROMFS_PATH/lib/libupnp.so $ROMFS_PATH/lib -fr 
		#cp $PLATFORM_ROMFS_PATH/lib/libshared.so $ROMFS_PATH/lib -fr 
		cp ${PLATFORM_WIFI_ROMFS_PATH}/lib/libshared.so ${ROMFS_PATH}/lib -fr 
	fi
fi
if [ "$CONFIG_VPN" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/lib/libvpnmsg.so $ROMFS_PATH/lib -fr 
	if [ "$CONFIG_VPN_PPTP" == "y" ] || [ "$CONFIG_NET_WAN_PPTP" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/lib/pptp.so $ROMFS_PATH/lib -fr 
	fi
	if [ "$CONFIG_VPN_IPSEC" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/lib/libcrypto.so.1.0.0 $ROMFS_PATH/lib -fr  
		if [ "$OPEN_SSL_SUPPORT" == "y" ] 		
		then
			cp $PLATFORM_ROMFS_PATH/lib/libssl.so $ROMFS_PATH/lib -fr 
		fi
		cp $PLATFORM_ROMFS_PATH/lib/librt.so.0 $ROMFS_PATH/lib -fr
	fi
fi

###########copy platform sbin to product romfs################### 
cp $PLATFORM_ROMFS_PATH/sbin/* $ROMFS_PATH/sbin -fr

###########copy platform usr to product romfs###################
cp $PLATFORM_ROMFS_PATH/usr/bin/* $ROMFS_PATH/usr/bin -fr 2> /dev/null
if [ "$CONFIG_APPS_IPTABLES_1412" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/usr/lib/libip4tc.so.0.0.0 $ROMFS_PATH/usr/lib -fr
	cp $PLATFORM_ROMFS_PATH/usr/lib/libiptc.so.0.0.0 $ROMFS_PATH/usr/lib -fr
	cp $PLATFORM_ROMFS_PATH/usr/lib/libxtables.so.7.0.0 $ROMFS_PATH/usr/lib -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/xtables-multi $ROMFS_PATH/usr/sbin/ -fr
fi

if [ "$CONFIG_APPS_IPTABLES" == "y" -o "$CONFIG_APPS_IPTABLES_144" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/iptables $ROMFS_PATH/bin/ -fr
	if [ "$CONFIG_IPV6_SUPPORT" == "y" ]
	then
		cp $PLATFORM_ROMFS_PATH/bin/ip6tables $ROMFS_PATH/bin/ -fr
	fi
fi

if [ "$CONFIG_EBTABLES_TOOL" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/usr/lib/libebtc.so $ROMFS_PATH/usr/lib -fr 
fi

if [ "$CONFIG_WIFI" == "y" ]
then
	if [ "$CONFIG_CHIP_VENDER" == "broadcom" ]
	then
		#cp $PLATFORM_ROMFS_PATH/usr/lib/libbcm.so $ROMFS_PATH/usr/lib -fr
		#cp $PLATFORM_ROMFS_PATH/usr/lib/libnvram.so $ROMFS_PATH/usr/lib -fr
		#cp $PLATFORM_ROMFS_PATH/usr/lib/libbcmcrypto.so $ROMFS_PATH/usr/lib -fr

		cp ${PLATFORM_WIFI_ROMFS_PATH}/lib/libbcm_util.so ${ROMFS_PATH}/usr/lib/libbcm.so -fr
		cp ${PLATFORM_WIFI_ROMFS_PATH}/lib/libnvram.so ${ROMFS_PATH}/usr/lib -fr
	fi
fi

cp $PLATFORM_ROMFS_PATH/usr/local/* $ROMFS_PATH/usr/local -fr
cp $PLATFORM_ROMFS_PATH/usr/share $ROMFS_PATH/usr/ -fr 2> /dev/null

if [ "$CONFIG_PPPoE_SERVER" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/usr/sbin/pppoe-server $ROMFS_PATH/usr/sbin/ -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/pppdv2 $ROMFS_PATH/usr/sbin/ -fr
fi

if [ "$CONFIG_WIFI" == "y" ]
then
	if [ "$CONFIG_CHIP_VENDER" == "broadcom" ]
	then
		cp $PLATFORM_WIFI_ROMFS_PATH/lib/modules/bcm_pcie_hcd.ko $ROMFS_PATH/lib/modules -fr
		cp $PLATFORM_WIFI_ROMFS_PATH/lib/modules/cfg80211.ko $ROMFS_PATH/lib/modules -fr
		cp $PLATFORM_WIFI_ROMFS_PATH/lib/modules/emf.ko $ROMFS_PATH/lib/modules -fr
		cp $PLATFORM_WIFI_ROMFS_PATH/lib/modules/hnd.ko $ROMFS_PATH/lib/modules -fr
		cp $PLATFORM_WIFI_ROMFS_PATH/lib/modules/igs.ko $ROMFS_PATH/lib/modules -fr
		cp $PLATFORM_WIFI_ROMFS_PATH/lib/modules/wl.ko $ROMFS_PATH/lib/modules -fr
		cp $PLATFORM_WIFI_ROMFS_PATH/lib/modules/wlcsm.ko $ROMFS_PATH/lib/modules -fr

		cp ${PLATFORM_WIFI_ROMFS_PATH}/bin/nvram ${ROMFS_PATH}/bin/sbin -fr
		cp ${PLATFORM_WIFI_ROMFS_PATH}/bin/wl ${ROMFS_PATH}/usr/sbin -fr
		cp ${PLATFORM_WIFI_ROMFS_PATH}/bin/acsd2 ${ROMFS_PATH}/usr/sbin/acsd -fr
		cp ${PLATFORM_WIFI_ROMFS_PATH}/bin/acs_cli2 ${ROMFS_PATH}/usr/sbin/acs_cli -fr

		#cp $PLATFORM_ROMFS_PATH/usr/sbin/nvram $ROMFS_PATH/usr/sbin -fr
		cp $PLATFORM_ROMFS_PATH/usr/sbin/wlconf $ROMFS_PATH/usr/sbin -fr
		cp $PLATFORM_ROMFS_PATH/usr/sbin/nas $ROMFS_PATH/usr/sbin -fr
		#cp $PLATFORM_ROMFS_PATH/usr/sbin/wl $ROMFS_PATH/usr/sbin -fr
		#cp $PLATFORM_ROMFS_PATH/usr/sbin/acsd $ROMFS_PATH/usr/sbin -fr
		#cp $PLATFORM_ROMFS_PATH/usr/sbin/acs_cli $ROMFS_PATH/usr/sbin -fr
		cp $PLATFORM_ROMFS_PATH/usr/sbin/td_acs_dbg $ROMFS_PATH/usr/sbin -fr
		if [ "$CONFIG_WIFI_EMF" == "y" ]
		then
			cp $PLATFORM_ROMFS_PATH/usr/sbin/emf $ROMFS_PATH/usr/sbin -fr
			cp $PLATFORM_ROMFS_PATH/usr/sbin/igs $ROMFS_PATH/usr/sbin -fr
		fi
		cp $PLATFORM_ROMFS_PATH/usr/sbin/udhcpd $ROMFS_PATH/usr/sbin -fr
	fi
fi

if [ "$CONFIG_SAMBA" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/usr/sbin/smbd $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/smbpasswd $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/nmbd $ROMFS_PATH/usr/sbin -fr
fi

if [ "$CONFIG_DEV_COMMUNICATION" == "y" ]
then
cp $PLATFORM_ROMFS_PATH/lib/libredis.so $ROMFS_PATH/lib -fr
cp $PLATFORM_ROMFS_PATH/lib/libcmdctl.so $ROMFS_PATH/lib -fr
fi

cp $PLATFORM_ROMFS_PATH/lib/libkm.so $ROMFS_PATH/lib -fr
cp $PLATFORM_ROMFS_PATH/bin/km_cmd $ROMFS_PATH/bin -fr
cp $PLATFORM_ROMFS_PATH/lib/libcm.so $ROMFS_PATH/lib -fr

if [ "$CONFIG_USB_SUPPORT" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/usr/sbin/smbd $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/smbpasswd $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/sbin/udevd $ROMFS_PATH/sbin -fr
	cp $PLATFORM_ROMFS_PATH/bin/vsftpd $ROMFS_PATH/bin -fr
	cp $PLATFORM_ROMFS_PATH/bin/ntfs-3g $ROMFS_PATH/bin -fr
	cp $PLATFORM_ROMFS_PATH/lib/modules/transcode.ko $ROMFS_PATH/lib/modules -fr
	cp $PLATFORM_ROMFS_PATH/lib/libntfs-3g.so.86 $ROMFS_PATH/lib -fr
	cp $PLATFORM_ROMFS_PATH/lib/modules/$KERNELRELEASE/build/fs/nls/nls_cp936.ko $ROMFS_PATH/lib/modules -fr
	cp $PLATFORM_ROMFS_PATH/lib/modules/$KERNELRELEASE/build/fs/nls/nls_cp950.ko $ROMFS_PATH/lib/modules -fr
fi

if [ "$CONFIG_IPV6_SUPPORT" == "y" ]
then
	cp $PLATFORM_ROMFS_PATH/bin/radvd $ROMFS_PATH/bin -fr
	cp $PLATFORM_ROMFS_PATH/bin/dhcp6s $ROMFS_PATH/bin -fr
	cp $PLATFORM_ROMFS_PATH/bin/dhcp6c $ROMFS_PATH/bin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/ipv6-up $ROMFS_PATH/etc_ro/ppp -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/ipv6-down $ROMFS_PATH/etc_ro/ppp -fr	
	cp $PLATFORM_ROMFS_PATH/usr/sbin/start_radvd.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/set_ipv6_default_route.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/dhcp6c_up.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/start_dhcp6c.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/start_dhcp6s.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/start_static_ipv6.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/set_ipv6_dns.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/start_pppoe_ipv6.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/del_if_global_addr.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/start_tunnel_6in4.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/start_tunnel_6to4.sh $ROMFS_PATH/usr/sbin -fr
	cp $PLATFORM_ROMFS_PATH/usr/sbin/start_tunnel_6rd.sh $ROMFS_PATH/usr/sbin -fr
	if [ "$CONFIG_MLDPROXY_SUPPORT" == "y" ] ; then
		cp $PLATFORM_ROMFS_PATH/bin/mldproxy $ROMFS_PATH/bin/ -fr
	fi
fi
#cp -f $PLATFORM_ROMFS_PATH/lib/modules/bm.ko $ROMFS_PATH/lib/modules
#cp -f $PLATFORM_ROMFS_PATH/lib/modules/mac_group.ko $ROMFS_PATH/lib/modules
#cp -f $PLATFORM_ROMFS_PATH/lib/modules/url_filter.ko $ROMFS_PATH/lib/modules
#cp -f $PLATFORM_ROMFS_PATH/lib/modules/mac_filter.ko $ROMFS_PATH/lib/modules
#cp -f $PLATFORM_ROMFS_PATH/lib/modules/nos.ko $ROMFS_PATH/lib/modules
#cp -f $PLATFORM_ROMFS_PATH/lib/modules/privilege_ip.ko $ROMFS_PATH/lib/modules
#cp -f $PLATFORM_ROMFS_PATH/lib/modules/ddos_ip_fence.ko $ROMFS_PATH/lib/modules
#cp -f $PLATFORM_ROMFS_PATH/lib/modules/arp_fence.ko $ROMFS_PATH/lib/modules
if [ "$CONFIG_PHY_CHECK" == "y" ]
then
	cp -f $PLATFORM_ROMFS_PATH/lib/modules/phy_check.ko $ROMFS_PATH/lib/modules
fi

if [ "$CONFIG_COMM_GPIO" == "y" ]
then
	cp -f $PLATFORM_ROMFS_PATH/lib/modules/gpio.ko $ROMFS_PATH/lib/modules
fi

#cp $PLATFORM_ROMFS_PATH/bin/tendaupgrade $ROMFS_PATH/bin/ -f

find $ROMFS_PATH/ -name ".svn" | xargs rm -fr

if [ "$CONFIG_BRIDGE_HANDLE_DHCP_OPTION" == "y" ]
then
	cp -f $PLATFORM_ROMFS_PATH/lib/modules/dhcp_options.ko $ROMFS_PATH/lib/modules
fi
