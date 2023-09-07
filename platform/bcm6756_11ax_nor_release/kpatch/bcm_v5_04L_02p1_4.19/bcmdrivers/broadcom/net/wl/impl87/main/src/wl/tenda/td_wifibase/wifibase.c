/*****************************************************************************
 文件名    : wifibase.c
 命名风格  : wifibase_ + 动作功能特性
 文件功能说明  ：在bcm大驱动中调用大驱动原有的接口实现相应功能接口，封装供wifibase模块使用
 修改历史  :
*****************************************************************************/

/*lint  -e10*/
/*lint  -e161*/
/*lint  -e163*/
/*lint  -e101*/
/*lint  -e63*/
/*lint  -e26*/
/*lint  -e322*/
/*lint  -e7*/
/*lint  -e48*/
/*lint  -e133*/
/*lint  -e18*/

#define __KERNEL_SYSCALLS__
#include <asm/uaccess.h>

#include "wifibase.h"
#include "wlc_types.h"
#include <wlc_iocv.h>
#include <802.11.h>
#include <wl_linux.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_lq.h>
#include <wlc_scb.h>

#if defined(IL_BIGENDIAN)
#include <bcmendian.h>
#define htod32(i) (bcmswap32(i))
#define htod16(i) (bcmswap16(i))
#define dtoh32(i) (bcmswap32(i))
#define dtoh16(i) (bcmswap16(i))
#define htodchanspec(i) htod16(i)
#define dtohchanspec(i) dtoh16(i)
#else
#define htod32(i) (i)
#define htod16(i) (i)
#define dtoh32(i) (i)
#define dtoh16(i) (i)
#define htodchanspec(i) (i)
#define dtohchanspec(i) (i)
#endif

/*lint  -e18*/
#ifdef CONFIG_WB_EASYMESH
int g_em_mesh_cfg = 0;
EXPORT_SYMBOL(g_em_mesh_cfg);

struct easymesh_driver_hook g_easymesh_driver_hook;
EXPORT_SYMBOL(g_easymesh_driver_hook);

struct td_em_fake_data g_td_em_fake_data = {0};//内部调试使用
EXPORT_SYMBOL(g_td_em_fake_data);

/* 设置调试等级 */
unsigned int g_td_debug = TD_DEBUG_ERROR | TD_DEBUG_INFO;
EXPORT_SYMBOL(g_td_debug);

/* 指定打印MAC, 00:00:00:00:00:00代表所有MAC */
struct ether_addr g_print_mac = {NULL_MAC};
EXPORT_SYMBOL(g_print_mac);
#endif

/*****************************************************************************
 函 数 名  : wifibase_get_assoclist
 功能描述  : 获取接口的关联mac表
 输入参数  : osif ：接口名
           data : 信息结构体
           data_len: 输入参数的长度
 输出参数  : data : maclist
 返 回 值: 成功 : WIFIBASE_OK 失败 : 小于0
 修改历史    :
  1.日    期   : 2022年7月22日
    作    者   : wzz
    修改内容   : 新生成函数
*****************************************************************************/
static int wifibase_get_assoclist(void *osif, void *data, unsigned int data_len)
{
    wlc_bsscfg_t *vap = NULL;
    struct maclist *buf = NULL;
    unsigned char (*maclist)[ETHER_ADDR_LEN] = (unsigned char (*)[ETHER_ADDR_LEN])data;
    struct maclist *macs = NULL;
    int i = 0;
    
    if (!osif || !data) {
        printk("%s, %d null ptr\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    vap = (wlc_bsscfg_t *)osif;
    if (!vap) {
        printk("%s, %d cfg is null\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    buf = (struct maclist*)kwb_malloc(WLC_IOCTL_MAXLEN);
    if (!buf) {
        printk("unable to kwb_malloc memory\n");
        return WIFIBASE_ERROR;
    }

    memset(buf, 0, WLC_IOCTL_MAXLEN);
    buf->count = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
    if (BCME_OK != wlc_ioctl(vap->wlc, WLC_GET_ASSOCLIST, (void *)buf, WLC_IOCTL_MAXLEN, vap->wlcif)) {
        kwb_free(buf);
        printk("%s,%d,wlc_ioctl is failed[cmd=%d]\n", __func__, __LINE__, WLC_GET_ASSOCLIST);
        return WIFIBASE_ERROR;
    }

    macs = (struct maclist *)buf;
    
    for (i = 0; i < macs->count; i++) {
        memcpy((void *)&maclist[i], (void *)&macs->ea[i], ETHER_ADDR_LEN);
    }
    
    kwb_free(buf);

    return WIFIBASE_OK;
}

/*****************************************************************************
 函 数 名  : wifibase_get_bandtype
 功能描述  : 获取bandtype和是否支持5gh的标志
 输入参数  : osif ：接口名
           data : 信息结构体
           data_len: 输入参数的长度
 输出参数  : data : band_type
 返 回 值  : 成功 WIFIBASE_OK ， 失败WIFIBASE_ERROR or WIFIBASE_NULL_POINTER

 修改历史    :
  1.日    期   : 2022年6月10日
    作    者   : wzz
    修改内容   : 新生成函数
*****************************************************************************/
static int wifibase_get_bandtype(void *osif, void *data, unsigned int data_len)
{
    int band_type = -1;
    int *band = NULL;
    wlc_bsscfg_t *vap = NULL;

    if (!osif || !data) {
        printk("%s, %d null ptr\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }
    
    band = (int *)data;
    vap = (wlc_bsscfg_t *)osif;

    if (!vap) {
        printk("%s, %d cfg is null\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }
    
    if (BCME_OK != wlc_ioctl(vap->wlc, WLC_GET_BAND, (void *)&band_type, sizeof(band_type), vap->wlcif)) {
        printk("%s,%d,wlc_ioctl is failed[cmd=%d]\n", __func__, __LINE__, WLC_GET_BAND);
        return WIFIBASE_ERROR;
    }

    // If band type is auto than update band type from band list
    if (WLC_BAND_AUTO == band_type) {
        int list[4];    // list[0] is the count, values at index 1, 2 ,3 contains band type.
        int i;

        if (BCME_OK != wlc_ioctl(vap->wlc, WLC_GET_BANDLIST, (void *)list, sizeof(list), vap->wlcif)) {
            printk("%s,%d,wlc_ioctl is failed[cmd=%d]\n", __func__, __LINE__, WLC_GET_BANDLIST);
            return WIFIBASE_ERROR;
        }
        list[0] = dtoh32(list[0]);
        list[1] = dtoh32(list[1]);
        list[2] = dtoh32(list[2]);
        list[3] = dtoh32(list[3]);

        /* list[0] is count, followed by 'count' bands */

        if (list[0] > 3) {
            list[0] = 3;
        }

        for (i = 1; i <= list[0]; i++) {
            if (WLC_BAND_6G == list[i]) {
                band_type = WLC_BAND_6G;
            }
            else if (WLC_BAND_5G == list[i]) {
                band_type = WLC_BAND_5G;
            }
            else if (WLC_BAND_2G == list[i]) {
                band_type = WLC_BAND_2G;
            }
            else {
                printk("invalid bandtype.\n");
            }
        }
    }

    *band = band_type;
    return WIFIBASE_OK;
}


/*****************************************************************************
 函 数 名: bcm_get_if_statis
 功能描述  : 获取radio级别信息
 输入参数  : osif ：接口名
           data : 信息结构体
           data_len: 输入参数的长度
 输出参数  : data : STA RSSI
 返 回 值: 成功 : WIFIBASE_OK 失败 : 小于0
 修改历史      :
  1.日    期   : 2021年03月13日
    作    者   : shijianhong
    修改内容   : 新生成函数
*****************************************************************************/
int bcm_get_if_statis(void *osif, void *data, unsigned int data_len)
{
    wl_if_stats_t  *cnt = NULL;
    kwb_if_info_t *if_info = (kwb_if_info_t *)data;
    wlc_bsscfg_t *cfg = (wlc_bsscfg_t *)osif;

    if (!cfg || !data) {
        printk("%s,%d, null ptr cfg : %pX, data : %pX\n", __func__, __LINE__, cfg, data);
        return WIFIBASE_NULL_POINTER;
    }

    cnt = (wl_if_stats_t  *)kwb_malloc(WLC_IOCTL_MAXLEN);
    if (!cnt) {
        printk("%s,%d,Failed to allocate buffer of %d bytes\n", __func__, __LINE__, WLC_IOCTL_MAXLEN);
        return WIFIBASE_ERROR;
     }

    strncpy((void *)cnt, "if_counters", WLC_IOCTL_MAXLEN);

    if (BCME_OK != wlc_ioctl(cfg->wlc, WLC_GET_VAR, (void *)cnt, WLC_IOCTL_MAXLEN, cfg->wlcif)) {
        kwb_free(cnt);
        printk("%s,%d,ioctl is failed[cmd=%d]\n", __func__, __LINE__, WLC_GET_VAR);
        return WIFIBASE_ERROR;
    }

    if_info->txframe = cnt->txframe;
    if_info->txbyte = cnt->txbyte;
    if_info->txerror = cnt->txerror;
    if_info->rxframe = cnt->rxframe;
    if_info->rxbyte = cnt->rxbyte;
    if_info->rxerror = cnt->rxerror;

    kwb_free(cnt);

    return WIFIBASE_OK;
}

static int wifibase_get_osif(void *osif, void *data, unsigned int data_len)
{
    struct net_device *dev = (struct net_device *)osif;
    wlc_bsscfg_t *cfg = NULL;
 
    if (!osif || !data) {
        printk("%s, %d null ptr osif %pX , data %pX \n", __func__, __LINE__, osif, data);
        return WIFIBASE_NULL_POINTER;
    }

    cfg = wlc_bsscfg_find_by_wlcif(WL_INFO_GET(dev)->wlc, WL_DEV_IF(dev)->wlcif);
    if (!cfg) {
        printk("%s, %d cfg is null\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    memcpy(data, &cfg, sizeof(void *));

    return WIFIBASE_OK;
}

/*****************************************************************************
 函 数 名  : wifibase_get_wlan_info
 功能描述  : 获取wlan因子参数
 输入参数  : 
 输出参数  : 
 返 回 值  : 成功 WIFIBASE_OK ， 失败WIFIBASE_ERROR or WIFIBASE_NULL_POINTER

 修改历史    :
  1.日    期   : 2020年6月21日
    作    者   : bcz
    修改内容   : 新生成函数
*****************************************************************************/
static int wifibase_get_wlan_info(void *osif, void *data, unsigned int data_len)
{
    wlc_bsscfg_t *vap = NULL;
    struct scb *scb = NULL;
    kwb_wlan_info_t *wlan_info = NULL;
    chanim_stats_t *chan_stat = NULL;
    unsigned char *mac = NULL;

    if (!osif || !data) {
        printk("%s, %d null ptr\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    wlan_info = (kwb_wlan_info_t *)data;
    mac = (unsigned char *)wlan_info->mac;
    if(!mac) {
        printk("%s, %d input mac is NULL\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    vap = (wlc_bsscfg_t *)osif;
    if (!vap) {
        printk("%s, %d vap is null\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    /* all vap has the same channel load, just get one */
    chan_stat = wlc_lq_chanspec_to_chanim_stats(vap->wlc->chanim_info,vap->current_bss->chanspec);
    if (chan_stat) {
        wlan_info->chan_busy = 100 - chan_stat->chan_idle; /* stats->chan_idle < 100% */
    }

    scb = wlc_scbfind(vap->wlc, vap, (struct ether_addr *)mac);
    if (scb) {
        wlan_info->rssi = scb->rssi; /*rssi is negative*/
        wlan_info->txrate = scb->scb_stats.tx_rate;
        wlan_info->rxrate = scb->scb_stats.rx_rate;
        wlan_info->channel = wf_chspec_ctlchan(vap->wlc->chanspec);
    }

    return WIFIBASE_OK;
}

/*****************************************************************************
 函 数 名  : wifibase_get_sta_num
 功能描述  : 获取指定接口下关联sta数(组网sta+关联用户sta)
 输入参数  :
 输出参数  : sta_count
 返 回 值  : 成功 WIFIBASE_OK ， 失败WIFIBASE_ERROR or WIFIBASE_NULL_POINTER

 修改历史    :
  1.日    期   : 2020年6月21日
    作    者   : bcz
    修改内容   : 新生成函数
*****************************************************************************/
static int wifibase_get_sta_num(void *osif, void *data, unsigned int data_len)
{
    wlc_bsscfg_t *vap = NULL;
    struct maclist* maclist = NULL;
    unsigned int *out_count = NULL;

    if (!osif || !data) {
        printk("%s, %d null ptr\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }
    out_count = (unsigned int *)data;

    vap = (wlc_bsscfg_t *)osif;
    if (!vap) {
        printk("%s, %d cfg is null\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    maclist = (struct maclist*)kwb_malloc(WLC_IOCTL_MAXLEN);
    if (!maclist) {
        printk("unable to kwb_malloc memory\n");
        return WIFIBASE_ERROR;
    }

    memset(maclist, 0, WLC_IOCTL_MAXLEN);
    maclist->count = (WLC_IOCTL_MAXLEN - sizeof(int)) / ETHER_ADDR_LEN;
    if (BCME_OK != wlc_ioctl(vap->wlc, WLC_GET_ASSOCLIST, (void *)maclist, WLC_IOCTL_MAXLEN, vap->wlcif)) {
        kwb_free(maclist);
        printk("%s,%d,wlc_ioctl is failed[cmd=%d]\n", __func__, __LINE__, WLC_GET_ASSOCLIST);
        return WIFIBASE_ERROR;
    }

    *out_count = (unsigned int)maclist->count;
    kwb_free(maclist);

    return WIFIBASE_OK;
}

static wb_fn_handle_t g_wifibase_function[] = { //原子函数模块管理
    {WIFIBASE_CMD_GET_IF_STATIS, bcm_get_if_statis, false},
    {WIFIBASE_CMD_GET_OSIF, wifibase_get_osif, false},
    {WIFIBASE_CMD_GET_WLAN_INFO, wifibase_get_wlan_info, false},
    {WIFIBASE_CMD_GET_STA_NUM, wifibase_get_sta_num, false},
    {WIFIBASE_CMD_GET_BANDTYPE, wifibase_get_bandtype, false},
    {WIFIBASE_CMD_GET_ASSOCLIST, wifibase_get_assoclist, false},
};

int kwb_bcm_kernel_init(wb_fn_ctx_t *ctx)
{
    if (!ctx) {
        printk("%s, %d null ptr is failed\n", __func__, __LINE__);
        return WIFIBASE_ERROR;
    }

    ctx->handle = g_wifibase_function;
    ctx->num = ARRY_NUM(g_wifibase_function);

    return WIFIBASE_OK;
}
EXPORT_SYMBOL(kwb_bcm_kernel_init);//wifibase内核重构模块初始化,以后有需要初始化的可以添加到该函数(例如wifibase向博通模块配置参数)


/*****************************************************************************
 函 数 名  : kwb_get_osifname_by_idx
 功能描述  : 依据idx获取osifname
 输入参数  :
 输出参数  : osifname
 返 回 值  : 成功 WIFIBASE_OK ， 失败WIFIBASE_ERROR or WIFIBASE_NULL_POINTER
 接口获取映射表关系：                 mib_if        -->     osif
                            wlan1.x        |      wl1
                非桥接模式：
                            wlan1.0/wlan1  |      wl1
                            wlan1.1        |      wl1.1
                            
                桥接模式:
                            wlan1          |      wl1
                            wlan1.0        |      wl1.8
                            wlan1.8        |      wl1
                其他radio接口转换同上表
 修改历史    :
  1.日    期   : 2021年7月14日
    作    者   : bcz
    修改内容   : 新生成函数
*****************************************************************************/
int kwb_get_osifname_by_idx(wifi_idx_t *widx, char *osifname)
{
    char prefix[] = "wl";
    int apsta = 0, wet = 0;
    char rfname[32] = {0};
    int vidx = 0, len = 0; 
    int os_name_len = 0, ret = 0;
    struct net_device *dev = NULL;
    long ptr = 0;
    wlc_bsscfg_t *cfg = NULL;

    if(!osifname || !widx){
        printk("[%s][%d]null point\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    snprintf(rfname, sizeof(osifname), "wl%d", widx->idx );
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    dev = dev_get_by_name(rfname);
#else
    dev = dev_get_by_name(&init_net, rfname);
#endif
    if(!dev) {
        printk("[%s][%d]dev is NULL \n",__func__,__LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    vidx = widx->vidx;
    sprintf(osifname, "%s%d", prefix, widx->idx);

    if(WIFI_BSSIDX_X == vidx) { /* wlan1.x --> wl1.8 */
        vidx = WIFI_BSSIDX_VXD;
        len = strlen(osifname);
        sprintf(osifname+len, ".%d", vidx);
        os_name_len = strlen(osifname);
        osifname[os_name_len] = '\0';
        dev_put(dev);
        return WIFIBASE_OK;
    }
    
    /* 依据radio获取wlc，继而获取工作模式 */
    ret = wifibase_get_osif((void *)dev,(void *)&ptr, 0);
    if(ret) {
        printk("wifibase_get_osif is failed\n");
        dev_put(dev);
        return WIFIBASE_ERROR;
    }
    cfg = (wlc_bsscfg_t *)ptr;
    if(!cfg) {
        printk("[%s][%d]cfg is nULL\n",__func__,__LINE__);
        dev_put(dev);
        return WIFIBASE_NULL_POINTER;
    }

    wlc_iovar_getint(cfg->wlc, "apsta", &apsta);
    wet = !!cfg->wlc->wet;
    /*             获取工作模式结束                 */
    if(!(apsta || wet)){                              //非桥接模式
        if(vidx > 0) {
            len = strlen(osifname);
            sprintf(osifname+len, ".%d", vidx);
        }else {

        }
    }else{                                             //桥接模式
        if((WIFI_BSSIDX_VXD == vidx) || (vidx < 0)) { //桥接模式wlan0.x/wlan0.8/wlan0 返回wl0

        }else if((vidx == WIFI_BSSIDX_0)) {           //桥接模式wlan0.0 返回wl0.8
            len = strlen(osifname);
            sprintf(osifname+len, ".%d", WIFI_BSSIDX_VXD);
        }else {
            len = strlen(osifname);
            sprintf(osifname+len, ".%d", vidx);
        }
    }

    os_name_len = strlen(osifname);
    osifname[os_name_len] = '\0';

    dev_put(dev);
    return WIFIBASE_OK;

}
EXPORT_SYMBOL(kwb_get_osifname_by_idx);


/*****************************************************************************
 函 数 名  : kwb_get_mibifname_by_idx
 功能描述  : 依据idx获取mibifname
 输入参数  :
 输出参数  : mibifname
 返 回 值  : 成功 WIFIBASE_OK ， 失败WIFIBASE_ERROR or WIFIBASE_NULL_POINTER
 接口获取映射表关系：                 osif         -->     mibif
                非桥接模式：
                            wl1           |      wlan1     
                            wl1.8         |      wlan1.x
                            wl1.1         |      wlan1.1

                桥接模式:
                            wl1.8         |      wlan1.0
                            wl1           |      wlan1.8
                            wl1.1         |      wlan1.1
                其他radio接口转换同上表

 修改历史    :
  1.日    期   : 2021年7月14日
    作    者   : bcz
    修改内容   : 新生成函数
*****************************************************************************/
int kwb_get_mibifname_by_idx(wifi_idx_t *widx, char *mibifname)
{
    char prefix[] = "wlan";
    int apsta = 0, wet = 0;
    char rfname[32] = {0};
    int vidx = 0, len = 0; 
    int os_name_len = 0, ret = 0;
    struct net_device *dev = NULL;
    long ptr = 0;
    wlc_bsscfg_t *cfg = NULL;

    if(!mibifname || !widx){
        printk("[%s][%d]null point\n", __func__, __LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    /* 依据radio获取wlc，继而获取工作模式 */
    snprintf(rfname, sizeof(mibifname), "wl%d", widx->idx );
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    dev = dev_get_by_name(rfname);
#else
    dev = dev_get_by_name(&init_net, rfname);
#endif
    if(!dev) {
        printk("[%s][%d]dev is NULL \n",__func__,__LINE__);
        return WIFIBASE_NULL_POINTER;
    }

    ret = wifibase_get_osif((void *)dev,(void *)&ptr, 0);
    if(ret) {
        printk("[%s][%d]wifibase_get_osif is failed\n",__func__,__LINE__);
        dev_put(dev);
        return WIFIBASE_ERROR;
    }

    cfg = (wlc_bsscfg_t *)ptr;
    if(!cfg) {
        printk("[%s][%d]cfg is nULL\n",__func__,__LINE__);
        dev_put(dev);
        return WIFIBASE_NULL_POINTER;
    }

    wlc_iovar_getint(cfg->wlc, "apsta", &apsta);
    wet = !!cfg->wlc->wet;
    /*             获取工作模式结束                 */
    vidx = widx->vidx;
    sprintf(mibifname, "%s%d", prefix, widx->idx);

    if(!(apsta || wet)){ //非桥接模式
        if(WIFI_BSSIDX_VXD == vidx) {
            len = strlen(mibifname);
            sprintf(mibifname+len, ".x");
        }else if(vidx > 0){
            len = strlen(mibifname);
            sprintf(mibifname+len, ".%d", vidx);
        }else{

        }
    }else{               //桥接模式
        if(WIFI_BSSIDX_VXD == vidx) { //wl1.8 -> wlan1.0
            len = strlen(mibifname);
            sprintf(mibifname+len, ".%d", WIFI_BSSIDX_0);
        }else if(vidx < 0) {           //wl1   -> wlan1.8
            len = strlen(mibifname);
            sprintf(mibifname+len, ".%d", WIFI_BSSIDX_VXD);
        }else {
            len = strlen(mibifname);
            sprintf(mibifname+len, ".%d", vidx);
        }
    }

    os_name_len = strlen(mibifname);
    mibifname[os_name_len] = '\0';

    dev_put(dev);
    return WIFIBASE_OK;

}
EXPORT_SYMBOL(kwb_get_mibifname_by_idx);

/*****************************************************************************
 函 数 名: kwb_get_osifname_idx
 功能描述  : 依据osifname获取idx
 输入参数  :    
 输出参数  : widx
 返 回 值  : 失败 WIFIBASE_ERROR  成功:WIFIBASE_OK
 修改历史      :
  1.日    期   : 2021年7月14日
    作    者   : bcz
    修改内容   : 新生成函数
*****************************************************************************/
int kwb_get_osifname_idx(const char *ifname, wifi_idx_t *widx)
{
    char *p = NULL, *q = NULL;
    int idx, vidx;

    if(!ifname || !widx){
        printk("%s(%d):NULL paramater!\n", __func__, __LINE__);
        return WIFIBASE_ERROR;
    }

    widx->idx  = WIFI_RFIDX_NONE;
    widx->vidx = WIFI_BSSIDX_NONE;
    p = strstr(ifname, "wl");

    if(p) {
        q = strchr(ifname, '.');
        if(q) {
            q++;
            if(('0' <= *q && *q <= '9')){ /* 判断是否为数字*/
                vidx = (int)simple_strtol(q, NULL, 10);
                if(vidx <= WIFI_BSSIDX_NONE || vidx > WIFI_BSSIDX_MAX) {
                    printk("%s(%d):Bad bss index!\n", __func__, __LINE__);
                    return WIFIBASE_ERROR;
                }else{
                    widx->vidx = vidx;
                }
            }else {  
                printk("%s(%d):Bad bss index!\n", __func__, __LINE__);
                return WIFIBASE_ERROR;
            }
        }

        p += strlen("wl");
        if(('0' <= *p && *p <= '9')){ /* 判断是否为数字*/
            idx = (int)simple_strtol(p, NULL, 10);
            
            if(idx <= WIFI_RFIDX_NONE || idx > WIFI_RFIDX_MAX){
                printk("%s(%d):Bad rf index:%d!\n", __func__, __LINE__, idx);
                return WIFIBASE_ERROR;
            }else {
                widx->idx = idx;
                return WIFIBASE_OK;
            }
        } else {
            printk("%s(%d):Bad rf index!\n", __func__, __LINE__);
            return WIFIBASE_ERROR;
        }
    }

    return WIFIBASE_ERROR;
}
EXPORT_SYMBOL(kwb_get_osifname_idx);

/*lint  +e10*/
/*lint  +e18*/
/*lint  +e161*/
/*lint  +e163*/
/*lint  +e101*/
/*lint  +e63*/
/*lint  +e26*/
/*lint  +e322*/
/*lint  +e7*/
/*lint  +e48*/
/*lint  +e133*/
