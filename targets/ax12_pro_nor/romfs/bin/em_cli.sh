#!/bin/sh

bbss_random=$RANDOM

cfmSet() {
	cfm set $1 $2
	sleep 0.5 # sleep 500ms
}

setBssDefault()
{
	mib_name=$1
	bss_type=$2

	cfmSet ${mib_name}_bss_brid 0
	cfmSet ${mib_name}_bss_enable 1
	cfmSet ${mib_name}_bss_hide 1
	cfmSet ${mib_name}_bss_macmode disabled
	cfmSet ${mib_name}_bss_macnum 32
	cfmSet ${mib_name}_bss_mcast2ucast 1
	cfmSet ${mib_name}_bss_ssid Tenda_EM_${bss_type}_${bbss_random}
	cfmSet ${mib_name}_bss_ssid_encode utf-8
	cfmSet ${mib_name}_bss_sta_denytype long
	cfmSet ${mib_name}_bss_wpapsk_crypto aes
	cfmSet ${mib_name}_bss_wpapsk_type psk2
	cfmSet ${mib_name}_bss_security wpapsk
	cfmSet ${mib_name}_bss_wpapsk_key 123456789
	cfmSet ${mib_name}_bss_wpapsk_rekey_time 0
	cfmSet ${mib_name}_bss_wps_enable 0
}

setMode() {
	mode=$1
	shift

	echo "Set easymesh mode to $mode"
	case "$mode" in
		"controller")
			cfmSet easymesh.enable 1
			cfmSet mesh.mode 1
			cfmSet easymesh.map_whole_network 1
			cfmSet mesh.device_name Tenda_Controller_$RANDOM
			cfmSet vpn.ser.pptpwanid 1
			cfmSet wlan0_workmode ap
			cfmSet wlan1_workmode ap
			cfmSet sys.workmode router
			cfmSet wans.flag 1
			cfmSet wlan0_enable 1
			cfmSet wlan0.0_bss_wps_enable 1
			cfmSet wlan0.0_bss_multiap_map 20 # fBSS
			cfmSet wlan0.2_bss_multiap_map 40 # bBSS
			setBssDefault wlan0.2 bBSS
			cfmSet wlan0.3_bss_multiap_map 20 # fBSS
			cfmSet wlan0.4_bss_multiap_map 20 # fBSS
			cfmSet wlan1_enable 1
			cfmSet wlan1.0_bss_wps_enable 1
			cfmSet wlan1.0_bss_multiap_map 20 # fBSS
			cfmSet wlan1.2_bss_multiap_map 40 # bBSS
			setBssDefault wlan1.2 bBSS
			cfmSet wlan1.3_bss_multiap_map 20 # fBSS
			cfmSet wlan1.4_bss_multiap_map 20 # fBSS
			cfmSet dhcps.leasetime 86400 # dhcps
			cfm post netctrl xmesh?op=12,wl_rate=7,apply_type=1
			;;
		"agent")
			bsta=$1
			case "$bsta" in
				"wlan0")
					cfmSet sys.workmode "client+ap"
					cfmSet wans.flag 1
					cfmSet sys.sched.wifi.enable 0
					cfmSet sys.powersleep.enable 0
					cfmSet easymesh.bsta_radio wlan0
					cfmSet wlan0.x_extend_multiap_map 80 # bSTA
					cfmSet wl.extra_chkHz 0
					cfmSet wlan0_workmode apclient
					cfmSet wlan1_workmode ap
					cfmSet wlan0.x_extend_enable 1
					cfmSet wlan0.x_extend_wps_enable 1
					cfmSet wlan1.x_extend_wps_enable 1
					cfmSet wl.extra_hand 0
					cfmSet apclient.syn_enable 0
					cfmSet apclient_syn_enable 0
					;;
				"wlan1")
					cfmSet sys.workmode "client+ap"
					cfmSet wans.flag 1
					cfmSet sys.sched.wifi.enable 0
					cfmSet sys.powersleep.enable 0
					cfmSet easymesh.bsta_radio wlan1
					cfmSet wlan1.x_extend_multiap_map 80 # bSTA
					cfmSet wl.extra_chkHz 1
					cfmSet wlan0_workmode ap
					cfmSet wlan1_workmode apclient
					cfmSet wlan1.x_extend_enable 1
					cfmSet wlan0.x_extend_wps_enable 1
					cfmSet wlan1.x_extend_wps_enable 1
					cfmSet wl.extra_hand 0
					cfmSet apclient.syn_enable 0
					cfmSet apclient_syn_enable 0
					;;
				"wired")
					cfmSet sys.workmode "ap"
					cfmSet easymesh.bsta_radio wired
					cfmSet wlan0_workmode ap
					cfmSet wlan1_workmode ap
					cfmSet wans.flag 0
					;;
				*)
					echo "Wrong bSTA '$bsta', bSTA must be wlan0, wlan1 or wired"
					exit 1
					;;
			esac

			cfmSet easymesh.enable 1
			cfmSet mesh.mode 2
			cfmSet mesh.device_name Tenda_Agent_$RANDOM
			cfmSet vpn.ser.pptpwanid 1
			cfmSet vpn.cli.pptpwanid 1
			cfmSet vpn.cli.l2tpwanid 1
			cfmSet wl.guest.dhcps_enable 0
			cfmSet wlan0.1_bss_enable 0
			cfmSet wlan1.1_bss_enable 0
			cfmSet wl.guest.24_enable 0
			cfmSet wl.guest.5_enable 0
			cfmSet dhcps.en 1
			cfmSet dhcps.leasetime 30 # dhcps
			cfmSet wlan0_enable 1
			cfmSet wlan0.0_bss_wps_enable 1
			cfmSet wlan0.0_bss_multiap_map 20 # fBSS
			cfmSet wlan0.2_bss_multiap_map 40 # bBSS
			setBssDefault wlan0.2 bBSS
			cfmSet wlan0.3_bss_multiap_map 20 # fBSS
			cfmSet wlan0.4_bss_multiap_map 20 # fBSS
			cfmSet wlan1_enable 1
			cfmSet wlan1.0_bss_wps_enable 1
			cfmSet wlan1.0_bss_multiap_map 20 # fBSS
			cfmSet wlan1.2_bss_multiap_map 40 # bBSS
			setBssDefault wlan1.2 bBSS
			cfmSet wlan1.3_bss_multiap_map 20 # fBSS
			cfmSet wlan1.4_bss_multiap_map 20 # fBSS
			cfmSet wan1_err_check 0
			cfmSet wan1_isonln 0
			cfmSet wan1_check 0
			cfmSet iptv.enable 0
			cfmSet iptv.stb.enable 0
			cfmSet igmp.enable 0
			cfmSet ipv6.enable 0
			reboot
			;;
		*)
			echo "Wrong mode '$mode', mode must be controller or agent"
			exit 1
			;;
	esac
}

setWifi() {
	mode=`cfm get easymesh.mode`
	if [ "$mode" != "1" ]; then
		echo "Set wifi bBSS or fBSS, only in controller mode"
		exit 1
	fi

	mib_name=`echo $1 | grep -E "wlan[01]\.[034]"`
	shift

	if [ "$mib_name" = "" ]; then
		echo "Wrong mib_name,  to run 'em_cli.sh help'"
		exit 1
	fi

	echo "Set wifi mib_name ($mib_name)"

	bss_type="fBSS"

	bss_enable=`cfm get ${mib_name}_bss_enable`
	if [ "$bss_enable" != "1" ]; then
		setBssDefault $mib_name $bss_type
	fi

	while true
	do
		param=$1
		shift

		if [ "$param" = "" ]; then
			break
		fi

		ssid=${param#ssid=}
		if [ "$ssid" != "${param}" ]; then
			cfmSet ${mib_name}_bss_ssid $ssid
			continue
		fi

		auth_type=${param#auth_type=}
		if [ "$auth_type" != "${param}" ]; then
			case "$auth_type" in
				"none")
					cfmSet ${mib_name}_bss_security none
					;;
				"wpapsk")
					cfmSet ${mib_name}_bss_security wpapsk
					cfmSet ${mib_name}_bss_wpapsk_type psk
					;;
				"wpa2psk")
					cfmSet ${mib_name}_bss_security wpapsk
					cfmSet ${mib_name}_bss_wpapsk_type psk2
					;;
				"wpawpa2psk")
					cfmSet ${mib_name}_bss_security wpapsk
					cfmSet ${mib_name}_bss_wpapsk_type "psk+psk2"
					;;
				*)
					echo "Wrong auth_type, ignore"
					;;
			esac
			continue
		fi

		key=${param#key=}
		if [ "$key" != "${param}" ]; then
			cfmSet ${mib_name}_bss_wpapsk_key $key
			continue
		fi

		crypto=${param#encry_type=}
		if [ "$crypto" != "${param}" ]; then
			case "$crypto" in
				"aes")
					cfmSet ${mib_name}_bss_wpapsk_crypto aes
					;;
				"tkip")
					cfmSet ${mib_name}_bss_wpapsk_crypto tkip
					;;
				"aes_tkip")
					cfmSet ${mib_name}_bss_wpapsk_crypto "tkip+aes"
					;;
				*)
					echo "Wrong encry_type, ignore"
					;;
			esac
			continue
		fi
	done

	radio=${mib_name:0:5}
	if [ "$radio" = "wlan0" ]; then
		cfm post netctrl wifi?op=3,wl_rate=5
	else
		cfm post netctrl wifi?op=3,wl_rate=2
	fi
}

setDebugLevel() {
	level=$1

	if [ "$level" != "0" -a "$level" != "1" -a "$level" != "2" -a "$level" != "3" ]; then
		echo "Wrong debug level '$level', level must be 0/1/2/3"
		exit 1
	fi

	cfmSet easymesh.debug_level $level
	cfm post netctrl easymesh?op=10
}

printHelp() {
	echo "Usage: em_cli.sh <command> [command arguments]"
	echo ""
	echo "    <command> can be any of the following:"
	echo "        - start Start easymesh"
	echo "        - restart Restart easymesh"
	echo "        - stop Stop easymesh"
	echo "        - disable Disable easymesh"
	echo "        - set_mode <mode> [bSTA] -- Set easymesh mode, will reboot the router"
	echo "            <mode> can be any of the following:"
	echo "                - controller -- Set easymesh to controller mode"
	echo "                - agent <bSTA> -- Set easymesh to agent mode, bSTA(wlan0/wlan1/wired) is required"
	echo "        - set_wifi <mib_name> [ssid=\"ssid\"] [auth_type=AUTH_TYPE] [key=\"key\"] [encry_type=ENCRY_TYPE] -- Set wifi fBSS, only in controller mode"
	echo "            <mib_name> can be any of the following:"
	echo "                - wlan0.0(wlan1.0) -- 5G(2.4G) first fBSS"
	echo "                - wlan0.3(wlan1.3) -- 5G(2.4G) second fBSS"
	echo "                - wlan0.4(wlan1.4) -- 5G(2.4G) third fBSS"
	echo "            AUTH_TYPE can be any of the following:"
	echo "                - none -- No encryption"
	echo "                - wpapsk -- WPA-PSK"
	echo "                - wpa2psk -- WPA2-PSK"
	echo "                - wpawpa2psk -- WPA/WPA2-PSK"
	echo "            ENCRY_TYPE can be any of the following:"
	echo "                - aes -- AES"
	echo "                - tkip -- TKIP"
	echo "                - aes_tkip -- AES&TKIP"
	echo "        - config_renew -- Send AP-Autoconfiguration Renew message"
	echo "        - set_dbg_level <level> -- Set the debug level of easymesh, level must be 0/1/2/3"
	echo "        - help -- Print the help info"
}

cmd=$1
shift

case $cmd in
	"start")
		cfmSet easymesh.enable 1
		cfm post netctrl easymesh?op=1
		;;
	"restart")
		cfmSet easymesh.enable 1
		cfm post netctrl easymesh?op=3
		;;
	"stop")
		cfm post netctrl easymesh?op=2
		;;
	"disable")
		cfm post netctrl easymesh?op=2
		cfmSet easymesh.enable 0
		;;
	"set_mode")
		setMode $@
		;;
	"set_wifi")
		setWifi $@
		;;
	"config_renew")
		cfm post netctrl easymesh?op=9
		;;
	"set_dbg_level")
		setDebugLevel $@
		;;
	"help")
		printHelp
		;;
	*)
		echo "Wrong command '$cmd'"
		printHelp
		;;
esac

