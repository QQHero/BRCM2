#include <stdio.h>
#include <string.h>
#include <kwb_ioctl.h>
#include <wlioctl.h>

#define WB_RSPEC_MCS_MASK		0x0000000F

int main(int argc, char *argv[])
{
    int ret,i = 0;
    unsigned char *ifname = NULL;
    wb_sta_chanim_info_t params;
    wb_ap_chanim_info_t ap_params;
    kwb_chan_info_t chan_info = {0};
    int ac_index,mcs;
    int rspec;
    wb_feature_info_t info;
    char *mac_str;
    char *end = NULL;
    int j = 10;

    if(!argv) {
        printf("error: %s[%d]: null pointer!\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    ifname = argv[1];
    memset(&rspec, 0, sizeof(wb_fixrate_info_t));
    memset(&info, 0, sizeof(wb_feature_info_t));
    memset(&params, 0, sizeof(wb_sta_chanim_info_t));
    memset(&ap_params, 0, sizeof(wb_ap_chanim_info_t));
    
    //sscanf(cb_node->sta_mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &chanim_stats.mac[0], &chanim_stats.mac[1], &chanim_stats.mac[2], &chanim_stats.mac[3], &chanim_stats.mac[4], &chanim_stats.mac[5]);
    //chanim_stats.mac = "42:7b:63:51:81:14"
    if (!argv[2]) {
        printf("no mac input\n");
        return WIFIBASE_NULL_POINTER;
    } else {
        mac_str = argv[2];
        if (mac_str && (INPUT_MAC_LENGTH != strlen(mac_str))) { //当mac_str不为空时校验输入的长度，为空时则默认全零
            printf("%s(%d) invalid param  mac_buf %p %d\n", __func__, __LINE__, mac_str,strlen(mac_str));
            return WIFIBASE_NULL_POINTER;
        }
        
        for (i = 0; mac_str && i < MAC_LENGTH; i++, mac_str = end + 1) {
            params.mac[i] = strtol(mac_str, &end, 16);
        }
    }

    params.stream_priority = 4;//获取ac队列(0-7)中sta的参数信息

    while(j--) {
        ret = uwb_get_sta_chanim_stats(ifname, &params);
        if (ret) {
           printf("%s[%d] uwb_get_sta_chanim_info error. ret = %d\n", __func__, __LINE__, ret);
           perror("Error:");
           return NULL;
        }
        printf("rx_pkts_retried(%d) rx_ucast_pkts(%d)\n", params.rx_pkts_retried, params.rx_ucast_pkts);

        sleep(5);

        info.speed_traffic = 4;//设置哪一个队列的协商速率
        rspec = params.rspec;
        mcs = params.be_mcs;
        mcs = mcs > 2 ? mcs-2 : mcs;
        rspec = (rspec & ~WB_RSPEC_MCS_MASK) | mcs;
        ret = uwb_fix_rate(ifname, &rspec, &info);
        if (ret) {
            printf("%s[%d] uwb_get_wme_ac_ip error. ret = %d\n", __func__, __LINE__, ret);
            perror("Error:");
        }

        printf("be_mcs(%d) mcs(%d) %d\n", params.be_mcs, mcs, info.speed_traffic);
        sleep(20);
    }

    return 0;
}


