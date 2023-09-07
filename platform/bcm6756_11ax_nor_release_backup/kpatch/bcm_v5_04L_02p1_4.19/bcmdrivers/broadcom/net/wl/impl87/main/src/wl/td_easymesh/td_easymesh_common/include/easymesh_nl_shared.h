#ifndef __TD_EASYMESH_NL_SHARED_H__
#define __TD_EASYMESH_NL_SHARED_H__

#define TD_EM_MACADDRLEN                        (6)
#define TD_EM_WLAN_SSID_MAXLEN                  (32)
#define TD_EM_MAX_CHANNELS_PER_OPERATING_CLASS  (24)
#define TD_EM_MAX_KEYLEN                        (65)
#define MAX_CHANNELS_PER_OPERATING_CLASS        (24)
#define MAX_CHANNE_NUM                         (24)
#define MAX_RADIOS_NUM                          (6)
#define MAX_STA_NUM                             (16)
#define MAX_TARGET_BSS_NUM                      (16)
#define MAX_REQ_IE_LEN                          (16)
#define MAX_AP_CHNEL_REPORT                     (4)
#define AP_CHANNELS_NUM                         (8)
#define SSID_MAXLEN                             (32)
#define MAX_BEACON_REPOR                       (64)
#define MAX_BEACON_SUBLEMENT_LENG                (226)
#define TD_EM_BUFF_LEN_16                       (16)
#define TD_EM_BUFF_LEN_32                       (32)
#define TD_EM_BUFF_LEN_64                       (64)

typedef enum netlink_event_type //add by qinke
{
    TD_EASYMESH_CLIENT_LEAVE            = 0, //offer to xmesh and easymesh
    TD_EASYMESH_CLIENT_JOIN             = 1, //offer to xmesh and easymesh
    TD_EASYMESH_CLIENT_CONNECT_FAIL     = 2, //offer to xmesh and easymesh
    TD_EASYMESH_WDS_ADDIF_BR            = 3, //offer to wserver
    TD_EASYMESH_EXTEND_DOWN             = 4, //offer to wserver
    TD_EASYMESH_BACKHAUL_STEER_RESPONSE = 11,//offer to easymesh
    TD_EASYMESH_STEER_STA_BTM           = 12,//offer to easymesh
    TD_EASYMESH_STEER_BTM_REPORT        = 13,//offer to easymesh
    TD_EASYMESH_CLIENT_CONNECT_NOTIFY   = 21,//offer to xmesh and easymesh
    TD_EASYMESH_CHANNEL_SCAN_RESULT     = 22,//offer to easymesh
    TD_EASYMESH_BEACON_METRIC_RSP       = 23,//offer to easymesh
}netlink_event_type_e;

typedef struct netlink_event_head
{
    netlink_event_type_e event;
}netlink_event_head_e;

typedef enum netlink_rson_code //add by qinke
{
    TD_EM_RSON_RESERVED                                    = 0,
    TD_EM_RSON_UNSPECIFIED                                 = 1,
    TD_EM_RSON_AUTH_NO_LONGER_VALID                        = 2,
    TD_EM_RSON_DEAUTH_STA_LEAVING                          = 3,
    TD_EM_RSON_INACTIVITY                                  = 4,
    TD_EM_RSON_UNABLE_HANDLE                               = 5,
    TD_EM_RSON_CLS2                                        = 6,
    TD_EM_RSON_CLS3                                        = 7,
    TD_EM_RSON_DISAOC_STA_LEAVING                          = 8,
    TD_EM_RSON_ASOC_NOT_AUTH                               = 9,
    TD_EM_RSON_DISASSOC_DUE_BSS_TRANSITION                 = 12,
    TD_EM_RSON_INVALID_IE                                  = 13,
    TD_EM_RSON_MIC_FAILURE                                 = 14,
    TD_EM_RSON_4WAY_HNDSHK_TIMEOUT                         = 15,
    TD_EM_RSON_GROUP_KEY_UPDATE_TIMEOUT                    = 16,
    TD_EM_RSON_DIFF_IE                                     = 17,
    TD_EM_RSON_MLTCST_CIPHER_NOT_VALID                     = 18,
    TD_EM_RSON_UNICST_CIPHER_NOT_VALID                     = 19,
    TD_EM_RSON_AKMP_NOT_VALID                              = 20,
    TD_EM_RSON_UNSUPPORT_RSNE_VER                          = 21,
    TD_EM_RSON_INVALID_RSNE_CAP                            = 22,
    TD_EM_RSON_IEEE_802DOT1X_AUTH_FAIL                     = 23,
    TD_EM_RSON_PMK_NOT_AVAILABLE                           = 24,
    TD_EM_RSON_USK_HANDSHAKE_TIMEOUT                       = 25,
    TD_EM_RSON_MSK_HANDSHAKE_TIMEOUT                       = 26,
    TD_EM_RSON_IE_NOT_CONSISTENT                           = 27,
    TD_EM_RSON_INVALID_USK                                 = 28,
    TD_EM_RSON_INVALID_MSK                                 = 29,
    TD_EM_RSON_INVALID_WAPI_VERSION                        = 30,
    TD_EM_RSON_INVALID_WAPI_CAPABILITY                     = 31,
    TD_EM_RSON_MESH_CHANNEL_SWITCH_REGULATORY_REQUIREMENTS = 65,
    TD_EM_RSON_MESH_CHANNEL_SWITCH_UNSPECIFIED             = 66
}netlink_rson_code_e;

typedef enum netlink_status_code //add by qinke
{
    TD_EM_STATUS_SUCCESSFUL                                = 0,
    TD_EM_STATUS_FAILURE                                   = 1,
    TD_EM_STATUS_CAP_FAIL                                  = 10,
    TD_EM_STATUS_NO_ASOC                                   = 11,
    TD_EM_STATUS_OTHER                                     = 12,
    TD_EM_STATUS_NO_SUPP_ALG                               = 13,
    TD_EM_STATUS_OUT_OF_AUTH_SEQ                           = 14,
    TD_EM_STATUS_CHALLENGE_FAIL                            = 15,
    TD_EM_STATUS_AUTH_TIMEOUT                              = 16,
    TD_EM_STATUS_UNABLE_HANDLE_STA                         = 17,
    TD_EM_STATUS_RATE_FAIL                                 = 18,
    TD_EM_STATUS_INVALID_PAIRWISE_CIPHER                   = 19,
    TD_EM_STATUS_R0KH_UNREACHABLE                          = 28,
    TD_EM_STATUS_ASSOC_REJ_TEMP                            = 30,
    TD_EM_STATUS_INSUFFICIENT_BANDWIDTH                    = 33,
    TD_EM_STATUS_POOR_CHANNEL_CONDITIONS                   = 34,
    TD_EM_STATUS_REQ_DECLINED                              = 37,
    TD_EM_STATUS_INVALID_IE                                = 40,
    TD_EM_STATUS_INVALID_AKMP                              = 43,
    TD_EM_STATUS_CIPER_REJECT                              = 46,
    TD_EM_STATUS_INVALID_USK                               = 47,
    TD_EM_STATUS_INVALID_MSK                               = 48,
    TD_EM_STATUS_INVALID_WAPI_VERSION                      = 49,
    TD_EM_STATUS_INVALID_WAPI_CAPABILITY                   = 50,
    TD_EM_STATUS_INVALID_FT_ACTION_FRAME_COUNT             = 52,
    TD_EM_STATUS_INVALID_PMKID                             = 53,
    TD_EM_STATUS_INVALID_MDIE                              = 54,
    TD_EM_STATUS_INVALID_FTIE                              = 55,
    TD_EM_STATUS_MESH_LINK_ESTABLISHED                     = 56,
    TD_EM_STATUS_MESH_UNDEFINE1                            = 57,
    TD_EM_STATUS_MESH_UNDEFINE2                            = 58,
    TD_EM_STATUS_REJ_BSS_TRANSITION                        = 82,
    TD_EM_STATUS_POSSIBLE_LOOP                             = 90
}netlink_status_code_e;


typedef struct client_association_event_tlv //add by qinke
{
    unsigned char sta_mac[TD_EM_MACADDRLEN];
    unsigned char bss_mac[TD_EM_MACADDRLEN];
}client_association_event_tlv_t;

typedef struct td_em_associated_sta_traffic_tlv //add by qinke
{
    unsigned int tx_bytes;
    unsigned int rx_bytes;
    unsigned int rx_packet;
    unsigned int tx_packet;
    unsigned int tx_fail;
    unsigned int rx_fail;
    unsigned int retranscount;
}td_em_associated_sta_traffic_tlv_t;

/********************************************************************************************/

typedef struct netlink_client_join //add by qinke
{
    netlink_event_head_e head;
    client_association_event_tlv_t mac;
}netlink_client_join_t; 

typedef struct netlink_client_leave //add by qinke
{
    netlink_event_head_e head;
    netlink_rson_code_e reason;
    client_association_event_tlv_t mac;
    td_em_associated_sta_traffic_tlv_t traffic;
}netlink_client_leave_t;

typedef struct netlink_client_connect_fail //add by qinke
{
    netlink_event_head_e head;
    netlink_status_code_e status;
    netlink_rson_code_e reason;
    client_association_event_tlv_t mac;
}netlink_client_connect_fail_t;

/********   chscan_ability end  *********/
typedef struct easymesh_clientcapability  //add by yjz
{
    netlink_event_head_e head;
    unsigned char   macaddr[TD_EM_MACADDRLEN];
    unsigned char   bssid[TD_EM_MACADDRLEN];
    unsigned char   framelength;
    unsigned char   framebody[];
}easymesh_clientcapability_tlv_t;

typedef struct wds_create_notify
{
    netlink_event_head_e type;
    char    ifname[16];
} wds_create_notify_t;

typedef struct wserver_netlink_head {
    netlink_event_head_e type;
    char *bufdata;
}wserver_netlink_head_t;

#endif
