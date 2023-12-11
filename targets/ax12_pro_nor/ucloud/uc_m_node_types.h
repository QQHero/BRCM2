#ifndef __UC_M_NODE_TYPES_H__
#define __UC_M_NODE_TYPES_H__

#define MAX_SN_LEN 32
#define MAX_LOCATE_LEN 64
#define IPADDR_STR_LEN              16
#define MAC_STR_LEN                 18
#define MAX_ASSOC_NUM  10
#define MAX_MODE_LEN 32
#define MAX_VERSION_LEN 64
#define COMMOM_LEN 32
#define MAX_MXP_NUM 9
#define MAX_PRODUCT_LEN 32
#define MAX_DEVICE_LEN 10
#define MAX_FREE_NODE_NUM 25

#define SET_NODE_X(pc,x)	\
	((pc)->mask |= (1 << x))
#define HAS_NODE_X(pc,x) \
	(((pc)->mask & (1 << x)) == (1 << x))

//节点信息
enum {
	ASSOC_NODE_INFO_WIRED = 0,
};


#define SET_ASSOC_NODE_INFO_WIRED(ack)	\
	SET_NODE_X(ack,ASSOC_NODE_INFO_WIRED)
#define HAS_ASSOC_NODE_INFO_WIRED(ack) \
	HAS_NODE_X(ack,ASSOC_NODE_INFO_WIRED)

typedef struct assoc_node_info_s {
	int mask;
	char assoc_sn[MAX_SN_LEN];
	int wl2g_rssi;
	int wl5g_rssi;
	int wired_en;
}assoc_node_info_t;

enum {
	NODE_MXP_ASSOC_UP = 0,
	NODE_MXP_VERSION,
	NODE_MXP_WIRED,
};


#define SET_NODE_MXP_ASSOC_UP(ack)	\
	SET_NODE_X(ack,NODE_MXP_ASSOC_UP)
#define HAS_NODE_MXP_ASSOC_UP(ack) \
	HAS_NODE_X(ack,NODE_MXP_ASSOC_UP)
#define SET_NODE_MXP_VERSION(ack)	\
	SET_NODE_X(ack,NODE_MXP_VERSION)
#define HAS_NODE_MXP_VERSION(ack) \
	HAS_NODE_X(ack,NODE_MXP_VERSION)
#define SET_NODE_MXP_WIRED(ack)	\
	SET_NODE_X(ack,NODE_MXP_WIRED)
#define HAS_NODE_MXP_WIRED(ack) \
	HAS_NODE_X(ack,NODE_MXP_WIRED)


typedef struct mxp_info_s {
	int mask;
	char serialNum[MAX_SN_LEN];
	int role;
	int status;
	char location[MAX_LOCATE_LEN];
	int led;
    char mode[MAX_MODE_LEN];
    char fwversion[MAX_VERSION_LEN];
    char ipaddr[IPADDR_STR_LEN];
    char ethaddr_l[MAC_STR_LEN];
    char ethaddr_r[MAC_STR_LEN];
    char bssid_nband[MAC_STR_LEN];
    char bssid_aband[MAC_STR_LEN];
	char bssid_sband[MAC_STR_LEN];
    int  assoc_num;
    assoc_node_info_t assoc_list[MAX_ASSOC_NUM];
    char time[COMMOM_LEN];
    char indication[COMMOM_LEN];
	assoc_node_info_t assoc_up;
	char version[MAX_VERSION_LEN];
	int wired_mesh;
}mxp_info_t;

typedef struct mesh_node_list_s {
	int n_cnt;
	mxp_info_t mxp_info[MAX_MXP_NUM];
}mesh_node_list_t;

typedef struct mxp_manage_s {
	char serialNum[MAX_SN_LEN];
	int opt;
}mxp_manage_t;

typedef struct max_manage_list_s {
	int n_mxp;
	mxp_manage_t mxp[MAX_MXP_NUM];
}max_manage_list_t;

typedef struct node_location_s {
	char serialNum[MAX_SN_LEN];
	char location[MAX_LOCATE_LEN];
}node_location_t;

typedef struct node_led_status_s {
	char serialNum[MAX_SN_LEN];
	int lev;
}node_led_status_t;

typedef struct manual_node_info_s {
	char serialNumMd5[MAX_SN_LEN];
}manual_node_info_t;

enum {
	NODE_FW_SEC_LEFT = 0,
};

#define SET_NODE_FW_SEC_LEFT(ack)	\
	SET_NODE_X(ack,NODE_FW_SEC_LEFT)
#define HAS_NODE_FW_SEC_LEFT(ack) \
	HAS_NODE_X(ack,NODE_FW_SEC_LEFT)

typedef struct fw_download_info_s {
	int mask;
	int fw_size;
	int recved;
	int sec_left;
}fw_download_info_t;

enum {
	NODE_DEVICE_PRODUCT = 0,
};

#define SET_NODE_DEVICE_PRODUCT(ack)	\
	SET_NODE_X(ack,NODE_DEVICE_PRODUCT)
#define HAS_NODE_DEVICE_PRODUCT(ack) \
	HAS_NODE_X(ack,NODE_DEVICE_PRODUCT)

typedef struct fw_device_info_s {
	int mask;
	char sn[MAX_SN_LEN];
	char version[MAX_VERSION_LEN];
	char product[MAX_PRODUCT_LEN];
}fw_device_info_t;

enum {
	NODE_UPGRADE_LANG = 0,
};

#define SET_NODE_UPGRADE_LANG(ack)	\
	SET_NODE_X(ack,NODE_UPGRADE_LANG)
#define HAS_NODE_UPGRADE_LANG(ack) \
	HAS_NODE_X(ack,NODE_UPGRADE_LANG)

typedef struct fw_upgreade_info_s {
	int mask;
	int n_device;
	fw_device_info_t device[MAX_DEVICE_LEN];
	int language_type;
}fw_upgreade_info_t;

enum {
	NODE_MULUPGRADE_LOAD = 0,
	NODE_MULUPGRADE_OP,
};

#define SET_NODE_MULUPGRADE_LOAD(ack)	\
	SET_NODE_X(ack,NODE_MULUPGRADE_LOAD)
#define HAS_NODE_MULUPGRADE_LOAD(ack) \
	HAS_NODE_X(ack,NODE_MULUPGRADE_LOAD)
#define SET_NODE_MULUPGRADE_OP(ack)	\
	SET_NODE_X(ack,NODE_MULUPGRADE_OP)
#define HAS_NODE_MULUPGRADE_OP(ack) \
	HAS_NODE_X(ack,NODE_MULUPGRADE_OP)


typedef struct fw_multi_upgrade_info_s {
	int mask;
	int op;
	int n_device;
	fw_device_info_t oom[MAX_DEVICE_LEN];
	fw_download_info_t download;
}fw_multi_upgrade_info_t;

typedef struct free_node_s {
	char sn[MAX_SN_LEN];
}free_node_t;

typedef struct free_node_list_s {
	int n_free_node;
	free_node_t free_node[MAX_FREE_NODE_NUM];
}free_node_list_t;

//fimily common ack
enum {
	NODE_ACK_NODE_LIST = 0,
	NODE_ACK_MXP,
	NODE_ACK_UPGRADE_INFO,
	NODE_ACK_FREE_NODE_LIST,
};

#define SET_NODE_ACK_NODE_LIST(ack)	\
	SET_NODE_X(ack,NODE_ACK_NODE_LIST)
#define HAS_NODE_ACK_NODE_LIST(ack) \
	HAS_NODE_X(ack,NODE_ACK_NODE_LIST)
#define SET_NODE_ACK_MXP(ack)	\
	SET_NODE_X(ack,NODE_ACK_MXP)
#define HAS_NODE_ACK_MXP(ack) \
	HAS_NODE_X(ack,NODE_ACK_MXP)
#define SET_NODE_ACK_UPGRADE_INFO(ack)	\
	SET_NODE_X(ack,NODE_ACK_UPGRADE_INFO)
#define HAS_NODE_ACK_UPGRADE_INFO(ack) \
	HAS_NODE_X(ack,NODE_ACK_UPGRADE_INFO)
#define SET_NODE_ACK_FREE_NODE_LIST(ack)	\
	SET_NODE_X(ack,NODE_ACK_FREE_NODE_LIST)
#define HAS_NODE_ACK_FREE_NODE_LIST(ack) \
	HAS_NODE_X(ack,NODE_ACK_FREE_NODE_LIST)

	
typedef struct node_common_ack_s {
	int mask;
	int err_code;
	mxp_info_t mxp_info;
	mesh_node_list_t node_list;
	fw_multi_upgrade_info_t upgrade_info;
	free_node_list_t free_node_list;
}node_common_ack_t;

#endif
