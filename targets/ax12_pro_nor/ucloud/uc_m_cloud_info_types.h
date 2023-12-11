#ifndef __UC_M_CLOUD_INFO_TYPES_H__
#define __UC_M_CLOUD_INFO_TYPES_H__
#include "typedefs.h"
#define SET_CLOUD_INFO_X(cinfo,x) \
	((cinfo)->mask |= (1 << x))
#define UNSET_CLOUD_INFO_X(cinfo,x) \
       ((cinfo)->mask &= (~(1 << x)))
#define HAS_CLOUD_INFO_X(cinfo,x) \
	(((cinfo)->mask & (1 << x)) == (1 << x))

typedef struct m_cloud_info_manage_en_s {
	int en;
}m_cloud_info_manage_en_t;

enum {
	_CLOUD_INFO_ACK_MANAGE_EN = 0,
};

#define SET_CLOUD_INFO_ACK_MANAGE_EN(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_ACK_MANAGE_EN)
#define HAS_CLOUD_INFO_ACK_MANAGE_EN(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_ACK_MANAGE_EN)
typedef struct m_cloud_info_ack_s {
	int mask;
	int err_code;
	m_cloud_info_manage_en_t manage_en;
}m_cloud_info_ack_t;

enum {
	_CLOUD_INFO_NODE_STATUS = 0,
	_CLOUD_INFO_NODE_LOCATION,
	_CLOUD_INFO_NODE_LED,
	_CLOUD_INFO_NODE_MODE,
	_CLOUD_INFO_NODE_FWVERSION,
	_CLOUD_INFO_NODE_IPADDR,
	_CLOUD_INFO_NODE_ETHADDR_L,
	_CLOUD_INFO_NODE_ETHADDR_R,
	_CLOUD_INFO_NODE_BSSID_NBAND,
	_CLOUD_INFO_NODE_BSSID_ABAND,
	_CLOUD_INFO_NODE_ASSOC_SN,
	_CLOUD_INFO_NODE_WL2G_RSSI,
	_CLOUD_INFO_NODE_WL5G_RSSI,
	_CLOUD_INFO_NODE_WIRED_EN,
	_CLOUD_INFO_NODE_BSSID_SBAND
};

#define UNSET_CLOUD_INFO_NODE_LOCATION(cinfo) \
       UNSET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_LOCATION)
#define UNSET_CLOUD_INFO_NODE_LED(cinfo) \
       UNSET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_LED)
#define UNSET_CLOUD_INFO_NODE_MODE(cinfo) \
       UNSET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_MODE)

#define SET_CLOUD_INFO_NODE_STATUS(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_STATUS)
#define SET_CLOUD_INFO_NODE_LOCATION(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_LOCATION)
#define SET_CLOUD_INFO_NODE_LED(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_LED)
#define SET_CLOUD_INFO_NODE_MODE(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_MODE)
#define SET_CLOUD_INFO_NODE_FWVERSION(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_FWVERSION)
#define SET_CLOUD_INFO_NODE_IPADDR(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_IPADDR)
#define SET_CLOUD_INFO_NODE_ETHADDR_L(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_ETHADDR_L)
#define SET_CLOUD_INFO_NODE_ETHADDR_R(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_ETHADDR_R)
#define SET_CLOUD_INFO_NODE_BSSID_NBAND(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_BSSID_NBAND)
#define SET_CLOUD_INFO_NODE_BSSID_ABAND(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_BSSID_ABAND) 
#define SET_CLOUD_INFO_NODE_BSSID_SBAND(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_BSSID_SBAND)
#define SET_CLOUD_INFO_NODE_ASSOC_SN(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_ASSOC_SN)
#define SET_CLOUD_INFO_NODE_WL2G_RSSI(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_WL2G_RSSI)
#define SET_CLOUD_INFO_NODE_WL5G_RSSI(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_WL5G_RSSI)
#define SET_CLOUD_INFO_NODE_WIRED_EN(cinfo) \
	SET_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_WIRED_EN) 

#define HAS_CLOUD_INFO_NODE_STATUS(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_STATUS)
#define HAS_CLOUD_INFO_NODE_LOCATION(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_LOCATION)
#define HAS_CLOUD_INFO_NODE_LED(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_LED)
#define HAS_CLOUD_INFO_NODE_MODE(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_MODE)
#define HAS_CLOUD_INFO_NODE_FWVERSION(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_FWVERSION)
#define HAS_CLOUD_INFO_NODE_IPADDR(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_IPADDR)
#define HAS_CLOUD_INFO_NODE_ETHADDR_L(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_ETHADDR_L)
#define HAS_CLOUD_INFO_NODE_ETHADDR_R(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_ETHADDR_R)
#define HAS_CLOUD_INFO_NODE_BSSID_NBAND(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_BSSID_NBAND)
#define HAS_CLOUD_INFO_NODE_BSSID_ABAND(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_BSSID_ABAND)
#define HAS_CLOUD_INFO_NODE_BSSID_SBAND(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_BSSID_SBAND)
#define HAS_CLOUD_INFO_NODE_ASSOC_SN(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_ASSOC_SN)
#define HAS_CLOUD_INFO_NODE_WL2G_RSSI(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_WL2G_RSSI)
#define HAS_CLOUD_INFO_NODE_WL5G_RSSI(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_WL5G_RSSI)
#define HAS_CLOUD_INFO_NODE_WIRED_EN(cinfo) \
	HAS_CLOUD_INFO_X(cinfo,_CLOUD_INFO_NODE_WIRED_EN) 
typedef struct _assoc_node_s {
	int mask;
	char assoc_sn[32];
	int wl2g_rssi;
	int wl5g_rssi;
	int wired_en;
}assoc_node_t;

typedef struct m_mesh_node_status_s {
	int	mask;
	char serialNum[32];
	int role;
	int status;
	char fwversion[32];
	char ipaddr[16];
	int assoc_sn_cnt;
	assoc_node_t assoc_list[];
}m_mesh_node_status_t;
typedef struct m_node_status_upload_s {
	int count;
	m_mesh_node_status_t node[];
}m_node_status_upload_t;
typedef struct m_mesh_node_u_s {
	int	mask;
	char serialNum[32];
	int opt;
	int	role;
	int status;
	int	led;
//	int wired_mesh;  
	char location[64];
	char mode[32];
	char fwversion[32];
	char ipaddr[16];
	char ethaddr_l[18];
	char ethaddr_r[18];
	char bssid_nband[18];
	char bssid_aband[18];
	char bssid_sband[18];
	int assoc_sn_cnt;
	assoc_node_t assoc_list[];
}m_mesh_node_u_t;
typedef struct m_cloud_info_mesh_node_u_s{
	int count;
	unsigned long	long timestamp;
	m_mesh_node_u_t node[];
}m_cloud_info_mesh_node_u_t;
typedef struct m_user_status_s {
	char ethaddr[18];
	int	uprate;
	int downrate;
	char ipaddr[16];
	int access;
	int signal;
}m_user_status_t;

typedef struct m_cloud_info_device_status_upload_s{
	int count;
	m_user_status_t status[];
}m_cloud_info_device_status_upload_t;
typedef struct m_mesh_node_s {
	int mask;
	char serialNum[32];
	char location[64];
	char mode[32];
	int	led;
//	int wired_mesh;   
}m_mesh_node_t;
typedef struct m_cloud_info_mesh_node_a_s{
	int count;
	int alreadexist;
	unsigned long	long timestamp;
	m_mesh_node_t node[];
}m_cloud_info_mesh_node_a_t;
typedef struct m_cloud_info_dev_config_time_s{
	unsigned long	long timestamp;
}m_cloud_info_dev_config_time_t;
typedef struct m_cloud_info_account_info_s {
	char account[512];
}m_cloud_info_account_info_t;
typedef struct m_cloud_info_upload_status_start_s{
	int interval;
	int keep;
}m_cloud_info_upload_status_start_t;
typedef struct m_cloud_info_web_auth_config_time_s{
	unsigned long	long  timestamp;
}m_cloud_info_web_auth_config_time_t;
#define MAX_VER_SIZE		(32)
#define MAX_DATE_LEN		(32)
#define MAX_DESC_LEN		(1024)
#define MAX_UPGRADE_CNT		(18)
enum {
	LANG_ZH_CN = 0, /* Chinese simple */
	LANG_EN,		/* English (maybe American) */
	LANG_ZH_TW,		/* Chinese traditional */
	LANG_MAX
};

typedef struct _inner_new_ver_info_q {
	char sn[32];
	char soft_ver[MAX_VER_SIZE];
	char product[32];
}inner_new_ver_info_q_t;

typedef struct _inner_multi_version_info_q {
	int32 device_cnt;
	inner_new_ver_info_q_t device[MAX_UPGRADE_CNT];
}m_cloud_info_version_multi_q_t;

typedef struct _inner_new_ver_info {
	char sn[32];
	char ver[MAX_VER_SIZE];
	char update_date[MAX_DATE_LEN];
	int size;
	char desc[LANG_MAX][512];
}inner_new_ver_info_t;

typedef struct _inner_multi_version_info {
	int32 device_cnt;
	inner_new_ver_info_t device[MAX_UPGRADE_CNT];
}m_cloud_info_version_multi_t;
#endif
