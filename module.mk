CFLAGS += -DCOMPANY_NAME=$(COMPANY_NAME)
CFLAGS += -DMAX_WAN_NUMBER=$(CONFIG_WAN_NUMBER)
ifneq ($(CONFIG_WAN_NUMBER),0)
CFLAGS += -DCONFIG_WAN_NUMBER=$(CONFIG_WAN_NUMBER)
endif
ifeq ($(CONFIG_CHIP_VENDER), broadcom)
CFLAGS += -DBROADCOM
endif
ifeq ($(CONFIG_CHIP_VENDER), realtek)
CFLAGS += -DREALTEK
endif
ifneq ($(CONFIG_MAX_PHY_PORT_NUM),0)
CFLAGS += -DCONFIG_MAX_PHY_PORT_NUM=$(CONFIG_MAX_PHY_PORT_NUM)
endif
#wan口方向
ifeq ($(CONFIG_WAN_LEFT),y)
	CFLAGS += -DCONFIG_WAN_LEFT
endif
#lan 口方向
ifeq ($(CONFIG_LAN_LEFT),y)
	CFLAGS += -DCONFIG_LAN_LEFT
endif
#wan/lan口从0开始
ifeq ($(CONFIG_WAN_LAN_COUNT_FROM_ZERO),y)
	CFLAGS += -DCONFIG_WAN_LAN_COUNT_FROM_ZERO
endif
#LAN口下标不从0/1开始，而是基于WAN口个数+1
ifeq ($(CONFIG_LAN_COUNT_BASE_ON_WAN),y)
	CFLAGS += -DCONFIG_LAN_COUNT_BASE_ON_WAN
endif
#手机页面支持
ifeq ($(CONFIG_MOBILE_WEB),y)
	CFLAGS += -DCONFIG_MOBILE_WEB
endif
#页面多国语言支持
CFLAGS += -DCONFIG_MULTI_LANGUAGE_SORFWARE 

#wan 口ip冲突是否自动修改
ifeq ($(CONFIG_WAN_IP_CONFLICT_AUTO_HANDLE),y)
	CFLAGS += -DCONFIG_WAN_IP_CONFLICT_AUTO_HANDLE
endif

ifeq ($(CONFIG_DHCPS_CHANGE_DOWN_LAN_PHY_CONNECT), y)
	CFLAGS += -DCONFIG_DHCPS_CHANGE_DOWN_LAN_PHY_CONNECT
endif

ifeq ($(CONFIG_DHCPS_LIST_MAX_NUM), y)
	CFLAGS += -DCONFIG_DHCPS_LIST_MAX_NUM
endif

ifeq ($(CONFIG_CWMPD), y)
	CFLAGS += -DCONFIG_CWMPD
endif

#dhcps服务器静态分配最多条数
CFLAGS += -DMAX_DHCPS_STATIC_IP_NUM=$(CONFIG_DHCPS_STATIC_IP_NUM)
#dhcp客户端最大数
CFLAGS += -DMAX_DHCP_CLIENT_NUM=$(CONFIG_MAX_DHCP_CLIENT)
#静态路由最大添加数
CFLAGS += -DCONFIG_MAX_STATIC_ROUTE_NUM=$(CONFIG_MAX_STATIC_ROUTE_NUM)

#nat最大连接数
CFLAGS += -DCONFIG_NAT_SESSION_NUMBER=$(CONFIG_NAT_SESSION_NUMBER)
CFLAGS += -DKERNELRELEASE=\"$(KERNELRELEASE)\"


ifeq ($(CONFIG_URL_MTD),y)
	CFLAGS+=-DCONFIG_URL_MTD
endif

ifeq ($(CONFIG_BEHAVIOR_MANAGER), y)
	#支持IP组条数
	ifneq ($(CONFIG_GROUP_IP_NUMBER), )
		CFLAGS += -DMAX_GROUP_IP_NUMBER=$(CONFIG_GROUP_IP_NUMBER)
	endif

	ifneq ($(CONFIG_DEFAULT_IP_GROUP_NUMBER), )
		CFLAGS += -DPRE_DEFINE_IP_GROUP_NUM=$(CONFIG_DEFAULT_IP_GROUP_NUMBER)
	endif

	#支持时间组条数
	ifneq ($(CONFIG_GROUP_TIMER_NUMBER), )
		CFLAGS += -DMAX_GROUP_TIMER_NUMBER=$(CONFIG_GROUP_TIMER_NUMBER)
	endif
	
	ifneq ($(CONFIG_DEFAULT_TIME_GROUP_NUMBER), )
		CFLAGS += -DPRE_DEFINE_TIME_GROUP_NUM=$(CONFIG_DEFAULT_TIME_GROUP_NUMBER)
	endif
	
	#支持mac条数
	ifneq ($(CONFIG_FILTER_MAC_NUMBER), )
		CFLAGS += -DMAX_FILTER_MAC_NUMBER=$(CONFIG_FILTER_MAC_NUMBER)
	endif

	#最大IP-MAC绑定条目数
	ifneq ($(CONFIG_BIND_IPMAC_NUMBER),)
		CFLAGS += -DCONFIG_BIND_IPMAC_NUMBER=$(CONFIG_BIND_IPMAC_NUMBER)
	endif

	#支持端口条数
	ifneq ($(CONFIG_FILTER_IPPORT_NUMBER), )
		CFLAGS += -DMAX_FILTER_IPPORT_NUMBER=$(CONFIG_FILTER_IPPORT_NUMBER)
	endif
	#支持url条数
	ifneq ($(CONFIG_FILTER_URL_NUMBER), )
		CFLAGS += -DMAX_FILTER_URL_NUMBER=$(CONFIG_FILTER_URL_NUMBER)
	endif
	#支持应用条数
	ifneq ($(CONFIG_FILTER_LAYER7_NUMBER), )
		CFLAGS += -DMAX_FILTER_LAYER7_NUMBER=$(CONFIG_FILTER_LAYER7_NUMBER)
	endif
	#支持QQ号码数
	ifneq ($(CONFIG_FILTER_QQ_NUMBER), )
		CFLAGS += -DMAX_FILTER_QQ_NUMBER=$(CONFIG_FILTER_QQ_NUMBER)
	endif
endif

ifeq ($(CONFIG_NEW_NETCTRL),y)
	CFLAGS+=-DCONFIG_NEW_NETCTRL=1
endif
#wan接入方式定义
ifeq ($(CONFIG_NET_WAN_STATIC), y)
	CFLAGS += -DCONFIG_NET_WAN_STATIC=1
endif

ifeq ($(CONFIG_NET_WAN_DHCP), y)
	CFLAGS += -DCONFIG_NET_WAN_DHCP=1
endif

ifeq ($(CONFIG_NET_WAN_PPPOE), y)
	CFLAGS += -DCONFIG_NET_WAN_PPPOE=1
endif

ifeq ($(CONFIG_NET_WAN_PPTP), y)
	CFLAGS += -DCONFIG_NET_WAN_PPTP=1
endif

ifeq ($(CONFIG_NET_WAN_L2TP), y)
	CFLAGS += -DCONFIG_NET_WAN_L2TP=1
endif
ifeq ($(CONFIG_NET_DOUBLE_PPTP), y)
	CFLAGS += -DCONFIG_NET_DOUBLE_PPTP=1
endif
ifeq ($(CONFIG_NET_DOUBLE_L2TP), y)
	CFLAGS += -DCONFIG_NET_DOUBLE_L2TP=1
endif
ifeq ($(CONFIG_NET_DOUBLE_PPOE), y)
	CFLAGS += -DCONFIG_NET_DOUBLE_PPOE=1
endif

ifeq ($(CONFIG_NET_WAN_PPPOA), y)
	CFLAGS += -DCONFIG_NET_WAN_PPPOA=1
endif

ifeq ($(CONFIG_NET_WAN_IPOA), y)
	CFLAGS += -DCONFIG_NET_WAN_IPOA=1
endif

ifeq ($(CONFIG_NET_WAN_IPOE), y)
	CFLAGS += -DCONFIG_NET_WAN_IPOE=1
endif

ifeq ($(CONFIG_SYSTEM_SNTP), y)
CFLAGS += -DCONFIG_SYSTEM_SNTP=1
endif

ifeq ($(CONFIG_SWITCH_MISC_MODE), y)
CFLAGS += -DCONFIG_SWITCH_MISC_MODE=1
endif

#多wan策略
ifeq ($(CONFIG_NET_MULTI_WAN), y)
	CFLAGS += -DCONFIG_NET_MULTI_WAN=1
endif

#ebtables工具
ifeq ($(CONFIG_EBTABLES_TOOL),y)
	CFLAGS += -DCONFIG_EBTABLES_TOOL=1
endif
#支持不认证弹广告
ifeq ($(CONFIG_PORTAL_UNONLINE),y)
CFLAGS += -DCONFIG_PORTAL_UNONLINE
endif
#支持应用层的portal定时检测
ifeq ($(CONFIG_PORTAL_USERSPACE_TIME_CHECK),y)
CFLAGS += -DCONFIG_PORTAL_USERSPACE_TIME_CHECK
endif

#支持AC管理
ifeq ($(CONFIG_AC_MANAGEMENT_V2),y)
	CFLAGS += -DCONFIG_AC_MANAGEMENT_V2
	CFLAGS += -DCONFIG_MAX_CLI_AP_NUM=$(CONFIG_MAX_CLI_AP_NUM)
	CFLAGS += -DAC_SQL_BLOCK_MTD=\"$(AC_SQL_BLOCK_MTD)\"
	CFLAGS += -DAC_POLICY_BLOCK_MTD=\"$(AC_POLICY_BLOCK_MTD)\"
endif

#支持 brcmnand

ifeq ($(CONFIG_BRCMNAND),y)
	CFLAGS += -DCONFIG_BRCMNAND
endif

ifeq ($(CONFIG_AC_QVLAN),y)
	CFLAGS += -DCONFIG_AC_QVLAN
endif
#支持dhcp服务器
ifeq ($(CONFIG_NET_DHCP), y)
	CFLAGS += -DCONFIG_NET_DHCP=1
endif

#支持dhcp服务器
ifeq ($(CONFIG_NET_DHCPFORAP), y)
	CFLAGS += -DCONFIG_NET_DHCPFORAP=1
endif

#支持dhcp静态绑定
ifeq ($(CONFIG_NET_DHCP_STATIC), y)
	CFLAGS += -DCONFIG_NET_DHCP_STATIC=1
endif
#支持mac地址克隆
ifeq ($(CONFIG_NET_PORT_CFG_MAC_CLONE), y)
	CFLAGS += -DCONFIG_NET_PORT_CFG_MAC_CLONE=1
endif

#支持端口模式可以配置
ifeq ($(CONFIG_NET_PORT_CFG_PORT_LINK_MODE), y)
	CFLAGS += -DCONFIG_NET_PORT_CFG_PORT_LINK_MODE=1
endif
#端口镜像
ifeq ($(CONFIG_NET_PORT_CFG_MIRROR), y)
	CFLAGS += -DCONFIG_NET_PORT_CFG_MIRROR=1
	CFLAGS += -DCONFIG_MIRROR_WATCH_LAN_PORT=$(CONFIG_MIRROR_WATCH_LAN_PORT)
endif


#支持WAN可以配置
ifeq ($(CONFIG_NET_PORT_CFG_WAN_NUMBER), y)
	CFLAGS += -DCONFIG_NET_PORT_CFG_WAN_NUMBER=1
endif

#支持虚拟服务器
ifeq ($(CONFIG_ADVANCE_VIRTUAL_SERVER), y)
	CFLAGS += -DCONFIG_ADVANCE_VIRTUAL_SERVER=1
endif

#支持端口段的映射
ifeq ($(CONFIG_PORT_RANGE_MAP), y)
	CFLAGS += -DCONFIG_PORT_RANGE_MAP=1
endif

#httpd支持SSL
ifeq ($(WEBS_SSL_SUPPORT), y)
	CFLAGS += -DWEBS_SSL_SUPPORT=1
endif

ifeq ($(OPEN_SSL_SUPPORT), y)
	CFLAGS += -DOPEN_SSL_SUPPORT=1
endif

#支持LAN口访问控制
ifeq ($(CONFIG_NET_CTL_WEB_ACCESS_LAN), y)
	CFLAGS += -DCONFIG_NET_CTL_WEB_ACCESS_LAN=1
endif
#支持WAN口访问控制
ifeq ($(CONFIG_NET_CTL_WEB_ACCESS_WAN), y)
	CFLAGS += -DCONFIG_NET_CTL_WEB_ACCESS_WAN=1
endif
#支持DMZ
ifeq ($(CONFIG_NET_DMZ), y)
	CFLAGS += -DCONFIG_NET_DMZ=1
endif
#地址伪装
ifeq ($(CONFIG_ADDRESS_MASQUERADE), y)
	CFLAGS += -DCONFIG_ADDRESS_MASQUERADE=1
endif
#支持 DDNS
ifeq ($(CONFIG_ADVANCE_DDNS), y)
	CFLAGS += -DCONFIG_ADVANCE_DDNS=1
endif

#支持智能省电
ifeq ($(CONFIG_SMART_POWER_MANAGEMENT), y)
	CFLAGS += -DCONFIG_SMART_POWER_MANAGEMENT=1
endif

ifeq ($(CONFIG_UCLOUD_FUNCTION), y)
	CFLAGS += -DCONFIG_UCLOUD_FUNCTION=1
endif

#支持LED控制
ifeq ($(CONFIG_SCHED_LED), y)
	CFLAGS += -DCONFIG_SCHED_LED=1
endif

ifeq ($(CONFIG_2_4G_LED_CONTROL), y)
	CFLAGS += -DCONFIG_2_4G_LED_CONTROL=1
endif
ifeq ($(CONFIG_5G_LED_CONTROL), y)
	CFLAGS += -DCONFIG_5G_LED_CONTROL=1
endif
ifeq ($(CONFIG_LED_SHARE_WITH_SYS), y)
	CFLAGS += -DCONFIG_LED_SHARE_WITH_SYS=1
endif
ifeq ($(CONFIG_WIFI_LED_TENDA_KERNEL_CONTROL), y)
	CFLAGS += -DCONFIG_WIFI_LED_TENDA_KERNEL_CONTROL=1
endif

#非中文版本去掉3322, 88ip, gnway
ifeq ($(CONFIG_WEB_LANG), cn)
	CFLAGS += -DCONFIG_WEB_LANG=\"CN\"
	CFLAGS += -DCONFIG_WEB_LANG_CN=1
#支持 3322 DDNS
ifeq ($(CONFIG_ADVANCE_3322), y)
	CFLAGS += -DCONFIG_ADVANCE_3322=1
endif
#支持 88IP DDNS
ifeq ($(CONFIG_ADVANCE_88IP), y)
	CFLAGS += -DCONFIG_ADVANCE_88IP=1
endif
#支持 金万维 DDNS
ifeq ($(CONFIG_ADVANCE_GNWAY), y)
	CFLAGS += -DCONFIG_ADVANCE_GNWAY=1
endif
else
	CFLAGS += -DCONFIG_WEB_LANG=\"EN\"
	CFLAGS += -DCONFIG_WEB_LANG_EN=1
endif

#支持 花生壳 DDNS
ifeq ($(CONFIG_ADVANCE_ORAY), y)
	CFLAGS += -DCONFIG_ADVANCE_ORAY=1
endif

#NOIP DDNS
ifeq ($(CONFIG_ADVANCE_NOIP), y)
	CFLAGS += -DCONFIG_ADVANCE_NOIP=1
endif

#DYNDNS DDNS
ifeq ($(CONFIG_ADVANCE_DYNDNS), y)
	CFLAGS += -DCONFIG_ADVANCE_DYNDNS=1
endif

#arp网管(酒店模式)
ifeq ($(CONFIG_ARP_GATEWAY), y)
	CFLAGS += -DCONFIG_ARP_GATEWAY=1
endif
#电子公告
ifeq ($(CONFIG_ADVANCE_WEB_NOTIFICATION), y)
	CFLAGS += -DCONFIG_ADVANCE_WEB_NOTIFICATION=1
endif

#支持UPNP
ifeq ($(CONFIG_ADVANCE_UPNP), y)
	CFLAGS += -DCONFIG_ADVANCE_UPNP=1
endif

#支持路由表
ifeq ($(CONFIG_ROUTE_TABLE), y)
	CFLAGS += -DCONFIG_ROUTE_TABLE=1
endif

#支持无线
ifeq ($(CONFIG_WIFI), y)
	CFLAGS += -DCONFIG_WIFI=1

	#支持WIFI6
	ifeq ($(CONFIG_WIFI6), y)
		CFLAGS += -DCONFIG_WIFI6=1
	endif

	ifeq ($(CONFIG_WIFI_BTN),y)
	CFLAGS += -DFUNCTION_WIFI_BTN
	endif
	
	ifeq ($(CONFIG_WIFI_CHINESE_SSID),y)
	CFLAGS += -DCONFIG_WIFI_CHINESE_SSID
	endif
	
	ifeq ($(CONFIG_WIFI_APSTA), y)
		CFLAGS += -DCONFIG_WIFI_APSTA=1
	endif
	
	ifeq ($(CONFIG_WIFI_AUTH_1X), y)
		CFLAGS += -DCONFIG_WIFI_AUTH_1X=1
	endif
	
	ifeq ($(CONFIG_WIFI_POWER_REGULATE), y)
		CFLAGS += -DCONFIG_WIFI_POWER_REGULATE=1
	endif
	
	ifeq ($(CONFIG_WIFI_WPS), y)
		CFLAGS += -DCONFIG_WIFI_WPS=1
	endif
	ifeq ($(CONFIG_WIFI_WISP), y)
		CFLAGS += -DCONFIG_WIFI_WISP=1
	endif
	ifeq ($(CONFIG_WIFI_ANTJ), y)
		CFLAGS += -DCONFIG_WIFI_ANTJ=1
	endif
	ifeq ($(CONFIG_WIFI_AC), y)
		CFLAGS += -DCONFIG_WIFI_AC=1
	endif
	
	ifeq ($(CONFIG_SCHED_WIFI), y)
		CFLAGS += -DCONFIG_SCHED_WIFI=1
	endif
	ifeq ($(CONFIG_WIFI_EMF), y)
		CFLAGS += -DCONFIG_WIFI_EMF=1
	endif	
	ifeq ($(CONFIG_WIFI_2_4G), y)
		CFLAGS += -DCONFIG_WIFI_2_4G=1
		ifeq ($(CONFIG_WIFI_2_4G_CHIP_BCM_ARM), y)
			CFLAGS += -DCONFIG_WIFI_2_4G_CHIP_BCM_ARM=1
		endif
		ifeq ($(CONFIG_WIFI_2_4G_CHIP_11AC), y)
			CFLAGS += -DCONFIG_WIFI_2_4G_CHIP_11AC=1
		endif
		#产测时需要知道修改频段MAC地址所对应的参数
		CFLAGS += -DCONFIG_WIFI_2_4G_CHIP_PRE=\"$(CONFIG_WIFI_2_4G_CHIP_PRE)\"
	endif
	ifeq ($(CONFIG_WIFI_5G), y)
		CFLAGS += -DCONFIG_WIFI_5G=1
		CFLAGS += -DCONFIG_WIFI_5G_CHIP_PRE=\"$(CONFIG_WIFI_5G_CHIP_PRE)\"
	endif
	ifeq ($(CONFIG_WIFI_GUEST), y)
		CFLAGS += -DCONFIG_WIFI_GUEST=1
	endif
	ifeq ($(CONFIG_RECV_SENSITIVITY), y)
		CFLAGS += -DCONFIG_RECV_SENSITIVITY=1
	endif
	ifeq ($(CONFIG_WL_PRIO_5G), y)
		CFLAGS += -DCONFIG_WL_PRIO_5G=1
		CFLAGS += -DPRIO_5G_FEATURE
	endif
	ifeq ($(CONFIG_PROBE_BROADCAST_SUPPRESSION), y)
		CFLAGS += -DCONFIG_PROBE_BROADCAST_SUPPRESSION=1
	endif
	ifeq ($(CONFIG_WIFI_FATAL_ERROR_MONITOR), y)
		CFLAGS += -DCONFIG_WIFI_FATAL_ERROR_MONITOR=1
	endif
	ifeq ($(CONFIG_WIFI_SUPPORT_SWITCH), y)
		CFLAGS += -DCONFIG_WIFI_SUPPORT_SWITCH=1
	endif

endif
ifeq ($(CONFIG_24G_PCI1),y)
CFLAGS += -DWLAN_PCI1_24G
endif
ifeq ($(CONFIG_24G_PCI2),y)
CFLAGS += -DWLAN_PCI2_24G
endif
ifeq ($(CONFIG_24G_PCI3),y)
CFLAGS += -DWLAN_PCI3_24G
endif
#支持的最大ssid数
	CFLAGS += -DMAX_2_4G_SSID_NUMBER=$(CONFIG_2_4G_MAX_SSID_NUMBER)
	CFLAGS += -DMAX_5G_SSID_NUMBER=$(CONFIG_5G_MAX_SSID_NUMBER)

	ifeq ($(CONFIG_AUTO_SSID_HIDE),y)
		CFLAGS += -DCONFIG_AUTO_SSID_HIDE=1
	endif
	ifeq ($(CONFIG_INTERFERENCE_MODE),y)
		CFLAGS += -DCONFIG_INTERFERENCE_MODE=1
	endif
	ifeq ($(CONFIG_RSSI_MANAGE),y)
		CFLAGS += -DCONFIG_RSSI_MANAGE=1
	endif

#支持品牌升级校验
ifeq ($(CONFIG_CHECK_FW_BY_VENDOR), y)
	CFLAGS += -DCONFIG_CHECK_FW_BY_VENDOR
endif
#支持3G
ifeq ($(CONFIG_3G), y)
	CFLAGS += -DCONFIG_3G=1
endif
#支持USB
ifeq ($(CONFIG_USB_SUPPORT), y)
	CFLAGS += -DCONFIG_USB_SUPPORT=1	
ifeq ($(CONFIG_NTFS_3G_SUPPORT), y)
	CFLAGS += -DCONFIG_NTFS_3G_SUPPORT=1
endif
ifneq ($(CONFIG_USB_PRINT_PRODUCT_NAME), "")
	CFLAGS += -DCONFIG_USB_PRINT_PRODUCT_NAME=\"$(CONFIG_USB_PRINT_PRODUCT_NAME)\"
else
	CFLAGS += -DCONFIG_USB_PRINT_PRODUCT_NAME=\"$(CONFIG_PRODUCT)\"
endif
endif
#支持Portal认证
ifeq ($(CONFIG_PORTAL_AUTH), y)
	CFLAGS += -DCONFIG_PORTAL_AUTH
	ifneq ($(CONFIG_PORTAL_WEB_AUTH_MAX_USR_NUM), )
	CFLAGS += -DCONFIG_PORTAL_WEB_AUTH_MAX_USR_NUM=$(CONFIG_PORTAL_WEB_AUTH_MAX_USR_NUM)
	endif
	ifneq ($(CONFIG_PORTAL_AUTH_WHITE_NUMBER), )
		CFLAGS += -DCONFIG_PORTAL_AUTH_WHITE_NUMBER=$(CONFIG_PORTAL_AUTH_WHITE_NUMBER)
	endif
	ifeq ($(CONFIG_PORTAL_URL_HIJACKING), y)
	CFLAGS += -DCONFIG_PORTAL_URL_HIJACKING
	endif
	ifeq ($(CONFIG_PORTAL_URL_WHITE_LIST), y)
	CFLAGS += -DCONFIG_PORTAL_URL_WHITE_LIST
	endif
	ifeq ($(CONFIG_PORTAL_JS_INJECT), y)
	CFLAGS += -DCONFIG_PORTAL_JS_INJECT
	endif
	#支持无线portal认证
	ifeq ($(CONFIG_PORTAL_MATCH_WL_IFNAME), y)
		CFLAGS += -DCONFIG_PORTAL_MATCH_WL_IFNAME
	endif
		#支持有线指定口portal认证
	ifeq ($(CONFIG_PORTAL_WIRED_IFNAME), y)
		CFLAGS += -DCONFIG_PORTAL_WIRED_IFNAME
	endif
	#支持不通过用户名密码进行认证
	ifeq ($(CONFIG_PORTAL_NO_AUTHENTICATE), y)
		CFLAGS += -DCONFIG_PORTAL_NO_AUTHENTICATE
	endif
	#支持无需内网认证
	ifeq ($(CONFIG_PORTAL_NO_LAN_AUTHENTICATE), y)
		CFLAGS += -DCONFIG_PORTAL_NO_LAN_AUTHENTICATE
	endif

	#支持内网无需认证

	#支持指定时间段上网
	ifeq ($(CONFIG_PORTAL_AUTH_DURING_SPECIFIC_TIME), y)
		CFLAGS += -DCONFIG_PORTAL_AUTH_DURING_SPECIFIC_TIME
	endif
	ifeq ($(CONFIG_PORTAL_MAC_WHITE_LIST), y)
		CFLAGS += -DCONFIG_PORTAL_MAC_WHITE_LIST
	endif
endif


ifeq ($(CONFIG_BEHAVIOR_MANAGER), y)

	#mac过滤
	ifeq ($(CONFIG_FILTER_MAC), y)
		CFLAGS += -DCONFIG_FILTER_MAC=1
	endif
	#ipmac绑定
	ifeq ($(CONFIG_FILTER_IPMAC), y)
		CFLAGS += -DCONFIG_FILTER_IPMAC=1
	endif
	#特权ip
	ifeq ($(CONFIG_PRIVILEGE_IP), y)
		CFLAGS += -DCONFIG_PRIVILEGE_IP=1
	endif
	#ip端口过滤
	ifeq ($(CONFIG_FILTER_IPPORT), y)
		CFLAGS += -DCONFIG_FILTER_IPPORT=1
	endif
	#url过滤
	ifeq ($(CONFIG_FILTER_URL), y)
		CFLAGS += -DCONFIG_FILTER_URL=1
	endif
	#app过滤
	ifeq ($(CONFIG_FILTER_APP), y)
		CFLAGS += -DCONFIG_FILTER_APP=1
	endif
	#账号管理
	ifeq ($(CONFIG_ACCOUNT_MANAGE), y)
		CFLAGS += -DCONFIG_ACCOUNT_MANAGE=1
	endif
	#connect limit
	ifeq ($(CONFIG_CONNECT_LIMIT), y)
        	CFLAGS += -DCONFIG_CONNECT_LIMIT=1
	endif
	#ip filter
	ifeq ($(CONFIG_IP_FILTER), y)
        	CFLAGS += -DCONFIG_IP_FILTER=1
	endif
endif

#
#ip组过滤
ifeq ($(CONFIG_FILTER_IPG),y)
	CFLAGS += -DCONFIG_FILTER_IPG__=1	
endif

ifeq ($(CONFIG_NAT_SPEEDUP), y)
	CFLAGS += -DCONFIG_NAT_SPEEDUP=1
endif



ifeq ($(CONFIG_SYSTEM_SNTP), y)
	CFLAGS += -DCONFIG_SYSTEM_SNTP=1
endif

#url分类过滤
ifeq ($(CONFIG_FILTER_WEBTYPE), y)
	CFLAGS += -DCONFIG_FILTER_WEBTYPE__=1
endif

#应用层深度包检测
ifeq ($(CONFIG_USERSPACE_DPI), y)
	CFLAGS += -DCONFIG_USERSPACE_DPI__=1
endif

#协议特征过滤
ifeq ($(CONFIG_FILTER_LAYER7), y)
	CFLAGS += -DCONFIG_FILTER_PROTO_LAYER7__=1
endif

#im过滤
ifeq ($(CONFIG_FILTER_IM), y)
	CFLAGS += -DCONFIG_FILTER_IM_LAYER7__=1
endif

#支持家长控制
ifeq ($(CONFIG_PARENT_CONTROL), y)
	CFLAGS += -DCONFIG_PARENT_CONTROL__=1
endif

#支持QOS
ifeq ($(CONFIG_QOS), y)
	CFLAGS += -DCONFIG_QOS=1
endif
ifeq ($(CONFIG_PORTAL_QOS), y)
        CFLAGS += -DCONFIG_PORTAL_QOS=1
endif

#支持TURBO_QOS带宽设置
ifeq ($(CONFIG_TURBO_QOS), y)
	CFLAGS += -DCONFIG_TURBO_QOS=1
endif

#支持TURBO_QOS自定义
ifeq ($(CONFIG_TURBO_QOS_AUTO), y)
	CFLAGS += -DCONFIG_TURBO_QOS_AUTO=1
endif

#IMQ_QO带宽设置
ifeq ($(CONFIG_IMQ_QOS), y)
	CFLAGS += -DCONFIG_IMQ_QOS=1
endif

#连接数限制
ifeq ($(CONFIG_NAT_SESSION_LIMIT), y)
	CFLAGS += -DCONFIG_NAT_SESSION_LIMIT=1
endif

#PPPoE-SERVER 认证
ifeq ($(CONFIG_PPPoE_SERVER),y)
	CFLAGS += -DCONFIG_PPPoE_SERVER=1

	ifneq ($(CONFIG_PPPOE_SESS_NUM), )
		CFLAGS += -DCONFIG_PPPOE_USER_NUM=$(CONFIG_PPPOE_SESS_NUM)
	endif
	CFLAGS += -DCONFIG_PPPOE_WHITE_MAC_NUMBER=20
	CFLAGS += -DCONFIG_PPPOE_WHITE_MAC_ADD_NUMBER=10
endif

#支持pppoe账号导出批量修改
ifeq ($(CONFIG_PPPoE_EXPORT_MODIFY),y)
	CFLAGS += -DCONFIG_PPPoE_EXPORT_MODIFY=1
endif
#支持pap认证
ifeq ($(CONFIG_PPP_PAP_AUTH),y)
	CFLAGS += -DCONFIG_PPP_PAP_AUTH=1
endif

#支持chap认证
ifeq ($(CONFIG_PPP_CHAP_AUTH),y)
	CFLAGS += -DCONFIG_PPP_CHAP_AUTH=1
endif
#支持web认证
ifeq ($(CONFIG_WEB_AUTH),y)
	CFLAGS += -DCONFIG_WEB_AUTH=1
endif

#支持server /service name修改
ifeq ($(CONFIG_PAGE_HAVE_SERV_NAME),y)
	CFLAGS += -DCONFIG_PAGE_HAVE_SERV_NAME
endif

#vpn支持
ifeq ($(CONFIG_VPN), y)
	CONFIG_VPN_CLIENT_NUM=$(CONFIG_VPN_CONNECT_NUMBER)

	CFLAGS += -DCONFIG_VPN=1
	CFLAGS += -DMAX_VPN_CONNECT_NUMBER=$(CONFIG_VPN_CONNECT_NUMBER)
	CFLAGS += -DMAX_VPN_USER_NUMBER=$(CONFIG_VPN_USER_NUMBER)
ifeq ($(CONFIG_VPN_IPSEC), y)
	CFLAGS += -DCONFIG_VPN_IPSEC=1
	CFLAGS += -DMAX_IPSEC_TUNNEL_NUMBER=$(CONFIG_IPSEC_TUNNEL_NUMBER)
endif
ifeq ($(CONFIG_VPN_PPTP), y)
	CFLAGS += -DCONFIG_VPN_PPTP=1
endif
ifeq ($(CONFIG_VPN_L2TP), y)
	CFLAGS += -DCONFIG_VPN_L2TP=1
endif
endif

#千兆 交换机
ifeq ($(CONFIG_NET_GMAC), y)
	CFLAGS += -DCONFIG_NET_GMAC=1
endif

#连接数限制
ifeq ($(CONFIG_NAT_SESSION_LIMIT), y)
	CFLAGS += -DCONFIG_NAT_SESSION_LIMIT=1
endif

#防攻击
ifeq ($(CONFIG_SAFE_ATTACK), y)
	CFLAGS += -DCONFIG_SAFE_ATTACK=1
endif

ifeq ($(CONFIG_SAFE_ARP), y)
	CFLAGS += -DCONFIG_SAFE_ARP=1
endif

#wan icmp drop
ifeq ($(CONFIG_WAN_ICMP_DROP), y)
	CFLAGS += -DCONFIG_WAN_ICMP_DROP=1
endif
#dmz
ifeq ($(CONFIG_NET_DMZ), y)
	CFLAGS += -DCONFIG_NET_DMZ=1
endif

#虚拟服务器
ifeq ($(CONFIG_ADVANCE_VIRTUAL_SERVER), y)
	CFLAGS += -DCONFIG_ADVANCE_VIRTUAL_SERVER=1
	
	ifeq ($(CONFIG_ADVANCE_VIRTUAL_SERVER_TIME_OUT), y)
		CFLAGS += -DCONFIG_ADVANCE_VIRTUAL_SERVER_TIME_OUT=1
	endif
	
	#端口映射最大添加数
	CFLAGS += -DCONFIG_MAX_PORT_MAP_NUM=$(CONFIG_MAX_PORT_MAP_NUM)
endif



#wan口访问控制
ifeq ($(CONFIG_NET_CTL_WEB_ACCESS_WAN), y)
	CFLAGS += -DCONFIG_NET_CTL_WEB_ACCESS_WAN=1
endif

#wan口故障检测控制
ifeq ($(CONFIG_WAN_STOPPAGE_CHECK), y)
	CFLAGS += -DCONFIG_WAN_STOPPAGE_CHECK=1
endif

ifeq ($(CONFIG_AUTO_SYNCHRO), y)
	CFLAGS += -DCONFIG_AUTO_SYNCHRO=1
endif	

#lan口访问控制
ifeq ($(CONFIG_NET_CTL_WEB_ACCESS_LAN), y)
	CFLAGS += -DCONFIG_NET_CTL_WEB_ACCESS_LAN=1
endif

#电子公告
ifeq ($(CONFIG_ADVANCE_WEB_NOTIFICATION), y)
	CFLAGS += -DCONFIG_ADVANCE_WEB_NOTIFICATION=1
endif


#1对1 NAT支持
ifeq ($(CONFIG_ADVANCE_P2P_NAT), y)
	CFLAGS += -DCONFIG_ADVANCE_P2P_NAT=1
endif

#支持Internet led指示
ifeq ($(CONFIG_INTERNET_LED), y)
	CFLAGS += -DCONFIG_INTERNET_LED=1
endif

#支持特征码文件升级
ifeq ($(CONFIG_SYSTEM_UPDATE_LAYER7_PROTOCOL), y)
	CFLAGS += -DCONFIG_SYSTEM_UPDATE_LAYER7_PROTOCOL=1
endif

#支持DSL模块
ifeq ($(CONFIG_DSL), y)

	CFLAGS += -DCONFIG_DSL=1


	ifeq ($(CONFIG_DSL_ANNEX_A), y)
		CFLAGS += -DCONFIG_DSL_ANNEX_A=1
	endif

	ifeq ($(CONFIG_DSL_ANNEX_B), y)
		CFLAGS += -DCONFIG_DSL_ANNEX_B=1
	endif

endif

#支持IPTV
ifeq ($(CONFIG_IPTV), y)
	CFLAGS += -DCONFIG_IPTV=1
endif
ifeq ($(CONFIG_IPTV_STB), y)
	CFLAGS += -DCONFIG_IPTV_STB=1
endif
ifeq ($(CONFIG_IGMPPROXY_SUPPORT),y)
	CFLAGS += -DCONFIG_IGMPPROXY_SUPPORT=1
	ifeq ($(CONFIG_PORT_SNOOPING),y)
		CFLAGS += -DCONFIG_PORT_SNOOPING
	endif
endif
ifeq ($(CONFIG_MLDPROXY_SUPPORT),y)
	CFLAGS += -DCONFIG_MLDPROXY_SUPPORT=1
endif
ifeq ($(CONFIG_BCM_IGMPPROXY), y)
	CFLAGS += -DCONFIG_BCM_IGMPPROXY=1
endif

ifeq ($(CONFIG_TR069), y)
	CFLAGS += -DCONFIG_TR069=1
endif

ifeq ($(CONFIG_IPV6), y)
	CFLAGS += -DCONFIG_IPV6=1
endif

#支持智能QOS
ifeq ($(CONFIG_TURBO_QOS_AUTO), y)
	CFLAGS += -DCONFIG_TURBO_QOS_AUTO=1
endif

ifeq ($(CONFIG_NOS_CONTROL), y)
        CFLAGS += -DCONFIG_NOS_CONTROL
endif

#支持USB
ifeq ($(CONFIG_USB), y)
	CFLAGS += -DCONFIG_USB=1
endif
ifeq ($(CONFIG_USB_SUPPORT), y)
	CFLAGS += -DCONFIG_USB_SUPPORT=1
endif
#支持SAMBA
ifeq ($(CONFIG_SAMBA), y)
	CFLAGS += -DCONFIG_SAMBA=1
endif

#支持USB打印机共享
ifeq ($(CONFIG_IPPD), y)
	CFLAGS += -DCONFIG_IPPD=1
endif

#星空极速
ifeq ($(CONFIG_CHINA_NET_CLIENT), y)
	CFLAGS += -DCONFIG_CHINA_NET_CLIENT=1
endif



#邮件密送
ifeq ($(CONFIG_EMAIL_CC), y)
	CFLAGS += -DCONFIG_EMAIL_CC=1
endif

#FTP SERVER
ifeq ($(CONFIG_FTP_SERVER), y)
	CFLAGS += -DCONFIG_FTP_SERVER=1
endif

#BT CLIENT
ifeq ($(CONFIG_BT_CLIENT), y)
	CFLAGS += -DCONFIG_BT_CLIENT=1
endif

#支持LAN口划分Vlan
ifeq ($(CONFIG_LAN_VLAN), y)
	CFLAGS += -DCONFIG_LAN_VLAN=1
	CFLAGS += -DCONFIG_MULTI_LAN=1
else
#多LAN口支持
ifeq ($(CONFIG_MULTI_LAN), y)
	CFLAGS += -DCONFIG_MULTI_LAN=1
endif
endif

#支持LAN口端口隔离（port vlan）
ifeq ($(CONFIG_LAN_PORT_VLAN), y)
	CFLAGS += -DCONFIG_LAN_PORT_VLAN=1
endif

#支持FORK
ifneq ($(CONFIG_MMU), n)
	CFLAGS += -DHAVE_FORK=1
else
	CFLAGS += -DNO_FORK=1
endif

#使用只读文件系统squashfs
ifeq ($(CONFIG_USE_SQUASHFS), y)
	CFLAGS += -DCONFIG_USE_SQUASHFS=1
endif

#web服务器选择
ifeq ($(CONFIG_APPWEB), y)
	CFLAGS += -DCONFIG_APPWEB=1
else
	CFLAGS += -DCONFIG_HTTPD=1
endif

######DEBUG#######
ifeq ($(CONFIG_IPMAC_GROUP_DEBUG), y)
	CFLAGS += -DCONFIG_IPMAC_GROUP_DEBUG=1
endif

ifeq ($(CONFIG_TIME_GROUP_DEBUG), y)
	CFLAGS += -DCONFIG_TIME_GROUP_DEBUG=1
endif

ifeq ($(CONFIG_IPV6_SUPPORT), y)
	CFLAGS += -DCONFIG_IPV6_SUPPORT=1

	ifeq ($(CONFIG_IPV6_NAT66), y)
		CFLAGS += -DCONFIG_IPV6_NAT66=1
	endif
endif

ifeq ($(SUPPORT_HTTP_IPV6),y)
	CFLAGS += -DSUPPORT_HTTP_IPV6=1
endif

#####CONFIG_BEHAVIOR_MANAGER####
ifeq ($(CONFIG_BEHAVIOR_MANAGER), y)
	CFLAGS += -DCONFIG_BEHAVIOR_MANAGER=1
endif

ifeq ($(CONFIG_PICTURE), y)
	CFLAGS += -DCONFIG_PICTURE=1
endif
ifeq ($(CONFIG_LAN_DHCPC), y)
	CFLAGS += -DCONFIG_LAN_DHCPC=1
endif

ifeq ($(CONFIG_QVLAN), y)
	CFLAGS += -DCONFIG_QVLAN=1
    CFLAGS += -DCONFIG_QVLAN_MAX_NUM=$(CONFIG_QVLAN_MAX_NUM)
ifeq ($(CONFIG_WIRED_QVLAN), y)
	CFLAGS += -DCONFIG_WIRED_QVLAN=1
endif

ifeq ($(CONFIG_WAN_QVLAN), y)
	CFLAGS += -DCONFIG_WAN_QVLAN=1
endif

ifeq ($(CONFIG_WIFI_QVLAN), y)
	CFLAGS += -DCONFIG_WIFI_QVLAN=1
ifeq ($(CONFIG_QVLAN_2_4G), y)
	CFLAGS += -DCONFIG_QVLAN_2_4G=1
endif
ifeq ($(CONFIG_QVLAN_5G), y)
	CFLAGS += -DCONFIG_QVLAN_5G=1
endif
endif
endif
ifeq ($(CONFIG_AP_MANAGE), y)
	CFLAGS += -DCONFIG_AP_MANAGE=1
	CFLAGS += -DLAN_DHCPC_FOR_AC=1
endif

#用于从SPI-flash启动,且有AC管理功能的产品,例如:G1,G3,M30,M50
ifeq ($(CONFIG_BOOT_FROM_SPIFLASH), y)
	CFLAGS += -DBOOT_FROM_SPIFLASH=1
ifeq ($(CONFIG_AP_MANAGE), y)
	CFLAGS += -DAC_SQL_BLOCK_MTD=\"$(AC_SQL_BLOCK_MTD)\"
	CFLAGS += -DAC_POLICY_BLOCK_MTD=\"$(AC_POLICY_BLOCK_MTD)\"
endif
endif

ifeq ($(CONFIG_DLNA_SERVER), y)
	CFLAGS += -DCONFIG_DLNA_SERVER=1
endif

ifeq ($(CONFIG_SNMP_SERVER), y)
	CFLAGS += -DCONFIG_SNMP_SERVER=1
endif
ifeq ($(CONFIG_SCHED_REBOOT), y)
	CFLAGS += -DCONFIG_SCHED_REBOOT=1
endif
ifeq ($(CONFIG_WEB_IDLE_TIME), y)
	CFLAGS += -DCONFIG_WEB_IDLE_TIME=1
endif
ifeq ($(CONFIG_DIAG_PING), y)
	CFLAGS += -DCONFIG_DIAG_PING=1
endif
ifeq ($(CONFIG_DIAG_TRACEROUTE), y)
	CFLAGS += -DCONFIG_DIAG_TRACEROUTE=1
endif
ifeq ($(CONFIG_LED_CONTROL),y)
CFLAGS += -DCONFIG_LED_CONTROL=1
endif
CFLAGS += -DCONFIG_PRODUCT=\"$(CONFIG_PRODUCT)\"
CFLAGS += -DCONFIG_PRODUCT_$(CONFIG_PRODUCT)=1

#2017-04-21需求变更,页面显示的产品名字以前是按品牌分,以后是按具体产品分,所以增加此宏.
CFLAGS += -DCONFIG_PRODUCT_NAME=\"$(CONFIG_PRODUCT_NAME)\"

ifeq ($(CONFIG_UI_QUICK_CONFIG), y)
	CFLAGS += -DCONFIG_UI_QUICK_CONFIG=1
endif

ifeq ($(CONFIG_UI_LOGIN_USERNAME_SUPPORT), y)
	CFLAGS += -DCONFIG_UI_LOGIN_USERNAME_SUPPORT=1
endif

CFLAGS += -DCONFIG_LOGIN_DOMAIN_NAME=\"$(CONFIG_LOGIN_DOMAIN_NAME)\"

CFLAGS += -DCONFIG_WIFI_CHIP=\"$(CONFIG_WIFI_CHIP)\"

CFLAGS += -DCONFIG_BRAND=\"$(CONFIG_BRAND)\"

ifeq ($(CONFIG_CFM_BACKUP), y)
CFLAGS += -DCONFIG_CFM_BACKUP
endif

#客户端URL记录
ifeq ($(CONFIG_URL_RCD),y)
	CFLAGS += -DCONFIG_URL_RCD
	ifeq ($(CONFIG_USB_URL_RCD),y)
	CFLAGS += -DCONFIG_USB_URL_RCD
	endif
endif

#云平台
ifeq ($(CONFIG_YUN_CLIENT), y)
	CFLAGS += -DCONFIG_YUN_CLIENT
	ifeq ($(CONFIG_YUN_URL_UPLOAD), y)
	CFLAGS += -DCONFIG_YUN_URL_UPLOAD
	endif
	ifeq ($(CONFIG_PORTAL_WEIXIN_AUTH), y)
	CFLAGS += -DCONFIG_PORTAL_WEIXIN_AUTH
	endif
	ifeq ($(CONFIG_PORTAL_CLOUD_AUTH), y)
	CFLAGS += -DCONFIG_PORTAL_CLOUD_AUTH
	endif
endif

ifeq ($(CONFIG_AP_PORTAL_AUTH), y)
CFLAGS += -DCONFIG_AP_PORTAL_AUTH
endif
#在线升级
ifeq ($(CONFIG_UCLOUD), y)
CFLAGS += -DCONFIG_UCLOUD
CFLAGS += -DCONFIG_UCLOUD_PASSWORD=\"$(CONFIG_UCLOUD_PASSWORD)\"
endif

#usb led 
ifeq ($(CONFIG_USB_LED_CONTROL), y)
CFLAGS += -DCONFIG_USB_LED_CONTROL
endif

ifneq ($(CONFIG_MAX_CLIENT_NUM),0)
CFLAGS += -DCONFIG_MAX_CLIENT_NUM=$(CONFIG_MAX_CLIENT_NUM)
endif

#系统日志显示最大条数
ifneq ($(CONFIG_MAX_SYS_LOG_NUM),)
ifneq ($(CONFIG_MAX_SYS_LOG_NUM),0)
CFLAGS += -DMAX_SYS_LOG_NUM=$(CONFIG_MAX_SYS_LOG_NUM)
endif
endif

ifeq ($(PRODUCT_VENDOR),ip-com)
	CFLAGS += -DVENDOR_IPCOM
endif

ifeq ($(PRODUCT_VENDOR),tenda)
	CFLAGS += -DVENDOR_TENDA
endif

ifeq ($(CONFIG_NOS_CONTROL), y)
	CFLAGS += -DCONFIG_NOS_CONTROL
endif

ifeq ($(CONFIG_FASTNAT_SWITCH), y)
	CFLAGS += -DCONFIG_FASTNAT_SWITCH
endif

ifeq ($(CONFIG_SWITCH_MODEL),bcm53125)
CFLAGS += -DSWITCH_BCM53125
endif

ifeq ($(CONFIG_SWITCH_MODEL),bcm5325e)
CFLAGS += -DSWITCH_BCM5325E
endif

ifeq ($(CONFIG_CHIP_VENDER),broadcom)
##Main Chip Selection##
ifeq ($(CONFIG_CHIP_MODEL),bcm4706)
CFLAGS += -DCONFIG_CHIP_MODEL_BCM4706
endif
ifeq ($(CONFIG_CHIP_MODEL),bcm4708)
CFLAGS += -DCONFIG_CHIP_MODEL_BCM4708
endif
ifeq ($(CONFIG_CHIP_MODEL),bcm4709c0)
CFLAGS += -DCONFIG_CHIP_MODEL_BCM4709
endif
ifeq ($(CONFIG_CHIP_MODEL),bcm47186)
CFLAGS += -DCONFIG_CHIP_MODEL_BCM47186
endif
ifeq ($(CONFIG_CHIP_MODEL),bcm47189)
CFLAGS += -DCONFIG_CHIP_MODEL_BCM47189
endif
ifeq ($(CONFIG_CHIP_MODEL),bcm5356c0)
CFLAGS += -DCONFIG_CHIP_MODEL_BCM5356C0
endif
ifeq ($(CONFIG_CHIP_MODEL),bcm53573)
CFLAGS += -DCONFIG_CHIP_MODEL_BCM53573
endif
endif

ifeq ($(CONFIG_TFCARD_HOTPLUG_SUPPORT), y)
	CFLAGS += -DCONFIG_TFCARD_HOTPLUG_SUPPORT
endif

#开启酒店模式后增加全网段IP组
ifeq ($(CONFIG_ADD_IPGROUP_BY_HOTEL_MODE), y)
	CFLAGS += -DCONFIG_ADD_IPGROUP_BY_HOTEL_MODE
endif

ifeq ($(CONFIG_AP_MODE), y)
	CFLAGS += -DCONFIG_AP_MODE
endif

ifeq ($(CONFIG_HW_NAT), y)
	CFLAGS += -DCONFIG_HW_NAT
endif

ifeq ($(CONFIG_CE_POWER), y)
	CFLAGS += -DCONFIG_CE_POWER
endif

ifeq ($(CONFIG_WIFI_POWER_ADJUST), y)
	CFLAGS += -DCONFIG_WIFI_POWER_ADJUST
endif
#支持串口打印信息控制
ifeq ($(CONFIG_CONSOLE_SWITCH), y)
	CFLAGS += -DCONFIG_CONSOLE_SWITCH
endif

#支持看门狗
ifeq ($(CONFIG_WATCHDOG_SWITCH), y)
	CFLAGS += -DCONFIG_WATCHDOG_SWITCH
endif

ifeq ($(CONFIG_CE_POWER_TSSI), y)
	CFLAGS += -DCONFIG_CE_POWER_TSSI
endif

#支持mac厂商从云端更新
ifeq ($(CONFIG_SCHED_MAC_VENDOR), y)
	CFLAGS += -DCONFIG_SCHED_MAC_VENDOR
endif

#支持设备离线日志
ifeq ($(CONFIG_SCHED_OFFLINE_LOG),y)
	CFLAGS += -DCONFIG_SCHED_OFFLINE_LOG
endif

ifeq ($(CONFIG_ATE_AES_CBC_ENCRYPT),y)
CFLAGS += -DATE_AES_CBC_ENCRYPT
endif

ifeq ($(CONFIG_SET_GUEST_SSID), y)
	CFLAGS += -DCONFIG_SET_GUEST_SSID
endif

ifeq ($(CONFIG_ALEXA_GUSET_WIFI_SET), y)
	CFLAGS += -DCONFIG_ALEXA_GUSET_WIFI_SET
endif

#支持Alexa设置LED状态
ifeq ($(CONFIG_ALEXA_LED_STATE_SET), y)
	CFLAGS += -DCONFIG_ALEXA_LED_STATE_SET
endif

#支持Alexa开启/关闭 WiFi Schedule
ifeq ($(CONFIG_ALEXA_WIFI_SCHEDULE_SET), y)
	CFLAGS += -DCONFIG_ALEXA_WIFI_SCHEDULE_SET
endif

#支持Alexa开启/关闭 WiFi WPS
ifeq ($(CONFIG_ALEXA_WIFI_WPS_SET), y)
	CFLAGS += -DCONFIG_ALEXA_WIFI_WPS_SET
endif

#支持Alexa设置 WiFi Power
ifeq ($(CONFIG_ALEXA_WIFI_POWER_SET), y)
	CFLAGS += -DCONFIG_ALEXA_WIFI_POWER_SET
endif

#支持Alexa设置 night mode
ifeq ($(CONFIG_ALEXA_NIGHT_MODE_SET), y)
	CFLAGS += -DCONFIG_ALEXA_NIGHT_MODE_SET
endif

#支持Alexa设置 WiFi Power
ifeq ($(CONFIG_ALEXA_GET_DEVICE_INFO), y)
	CFLAGS += -DCONFIG_ALEXA_GET_DEVICE_INFO
endif
#支持Alexa设置 night mode
ifeq ($(CONFIG_ALEXA_CHANGE_DEVICE_NETWORK), y)
	CFLAGS += -DCONFIG_ALEXA_CHANGE_DEVICE_NETWORK
endif
#支持海外可选ISP
ifeq ($(CONFIG_SELECT_ISP),y)
	CFLAGS += -DCONFIG_SELECT_ISP
endif

#支持强制升级
ifeq ($(CONFIG_SCHED_FORCE_UPGRADE),y)
	CFLAGS += -DCONFIG_SCHED_FORCE_UPGRADE
endif

#支持WAN口盲插
ifeq ($(CONFIG_WAN_AUTO_DETECT),y)
	CFLAGS += -DCONFIG_WAN_AUTO_DETECT
endif

#easymesh 支持
ifeq ($(CONFIG_TENDA_EASYMESH),y)
	CFLAGS += -DCONFIG_TENDA_EASYMESH
endif

#xmesh 支持
ifeq ($(CONFIG_TENDA_XMESH),y)
	CFLAGS += -DCONFIG_TENDA_XMESH
endif

#xmesh 支持
ifeq ($(CONFIG_PLATFORM_MSG),y)
	CFLAGS += -DCONFIG_PLATFORM_MSG
endif

#一键换机支持
ifeq ($(CONFIG_CBRR),y)
	CFLAGS += -DCONFIG_CBRR
endif

ifeq ($(CONFIG_WIFI_SUPPORT_APCLIENT),y)
	CFLAGS += -DCONFIG_WIFI_SUPPORT_APCLIENT
endif
ifeq ($(CONFIG_WIFI_SUPPORT_WISP),y)
	CFLAGS += -DCONFIG_WIFI_SUPPORT_WISP
endif

#支持HTTP长连接选项
ifeq ($(CONFIG_SUPPORT_HTTP_KEEP_ALIVE),y)
	CFLAGS += -DCONFIG_SUPPORT_HTTP_KEEP_ALIVE
endif

#syschk 故障检测
ifeq ($(CONFIG_TENDA_SYSCHK),y)
    CFLAGS += -DCONFIG_TENDA_SYSCHK
endif
