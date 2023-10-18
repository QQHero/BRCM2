/*****************************************************************************
Copyright (C), 吉祥腾达，保留所有版权
File name ：td_easymesh_opclass.c
Description : easymesh
Author ：qinke@tenda.cn
Version ：v1.0
Date ：2020.4.26
*****************************************************************************/
#include <linux/kernel.h>

#include "td_easymesh_dbg.h"
#include "easymesh_shared.h"
#include "td_easymesh_interface.h"
/*lint  -e19*/
/*lint  -e129*/
/*lint  -e10*/
/*lint  -e18*/
typedef enum td_em_channel_offect
{
    TD_EM_CHAN_OFFSET_SCN = 0, //正常信道
    TD_EM_CHAN_OFFSET_SCA = 1, 
    TD_EM_CHAN_OFFSET_SCB = 2
}td_em_channel_offect_e;

typedef struct td_em_op_class_map
{
    unsigned char op_class;
    td_em_bw_e ch_width;
    td_em_channel_offect_e sec20_offset;
    unsigned char ch_set[MAX_CHANNELS_PER_OPERATING_CLASS];
}td_em_op_class_map_t;

/* Us 操作类 */
static const td_em_op_class_map_t g_us_operating_class[] = {
    {1,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48}},
    {2,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {52, 56, 60, 64}},
    {4,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144}},
    {5,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {149, 153, 157, 161, 165}},
    {12,  TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}},
    {22,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {36, 44}},
    {23,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {52, 60}},
    {24,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {100, 108, 116, 124, 132, 140}},
    {25,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {149, 157}},
    {27,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {40, 48}},
    {28,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {56, 64}},
    {29,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {104, 112, 120, 128, 136, 144}},
    {31,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {153, 161}},
    {32,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {1, 2, 3, 4, 5, 6, 7}},
    {33,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {5, 6, 7, 8, 9, 10, 11}},
    {128, TD_EM_CHWIDTH80, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128,
           132, 136, 140, 144, 149, 153, 157, 161}},
    {129, TD_EM_CHWIDTH160, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128}},
    {130, TD_EM_CHWIDTH80_80, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128,
           132, 136, 140, 144, 149, 153, 157, 161}},
    {0, 0, 0, {0}},
};

/* Europe 操作类 */
static const td_em_op_class_map_t g_europe_operating_class[] = {
    {1,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48}},
    {2,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {52, 56, 60, 64}},
    {3,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}},
    {4,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}},
    {5,   TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {36, 44}},
    {6,   TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {52, 60}},
    {7,   TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {100, 108, 116, 124, 132}},
    {8,   TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {40, 48}},
    {9,   TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {56, 64}},
    {10,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {104, 112, 120, 128, 136}},
    {11,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {1, 2, 3, 4, 5, 6, 7, 8, 9}},
    {12,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {5, 6, 7, 8, 9, 10, 11, 12, 13}},
    {17,  TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {149, 153, 157, 161, 165, 169}},
    {128, TD_EM_CHWIDTH80, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128}},
    {129, TD_EM_CHWIDTH160, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128}},
    {130, TD_EM_CHWIDTH80_80, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128}},
    {0, 0, 0, {0}},
};

/* Japan 操作类 */
static const td_em_op_class_map_t g_japan_operating_class[] = {
    {1,   TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48}},
    {30,  TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}},
    {31,  TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {14}},
    {32,  TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {52, 56, 60, 64}},
    {34,  TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140}},
    {36,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {36, 44}},
    {37,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {52, 60}},
    {39,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {100, 108, 116, 124, 132}},
    {41,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {40, 48}},
    {42,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {56, 64}},
    {44,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {104, 112, 120, 128, 136}},
    {128, TD_EM_CHWIDTH80, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128}},
    {129, TD_EM_CHWIDTH160, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128}},
    {130, TD_EM_CHWIDTH80_80, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128}},
    {0, 0, 0, {0}},
};

/* Global 操作类 */
static const td_em_op_class_map_t g_global_operating_class[] = {
    {81,  TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}},
    {82,  TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {14}},
    {83,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {1, 2, 3, 4, 5, 6, 7, 8, 9}},
    {84,  TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {5, 6, 7, 8, 9, 10, 11, 12, 13}},
    {115, TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48}},
    {116, TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {36, 44}},
    {117, TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {40, 48}},
    {118, TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {52, 56, 60, 64}},
    {119, TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {52, 60}},
    {120, TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {56, 64}},
    {121, TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144}},
    {122, TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {100, 108, 116, 124, 132, 140}},
    {123, TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {104, 112, 120, 128, 136, 144}},
    {125, TD_EM_CHWIDTH20, TD_EM_CHAN_OFFSET_SCN,
          {149, 153, 157, 161, 165, 169}},
    {126, TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCA,
          {149, 157}},
    {127, TD_EM_CHWIDTH40, TD_EM_CHAN_OFFSET_SCB,
          {153, 161}},
    {128, TD_EM_CHWIDTH80, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128,
           132, 136, 140, 144, 149, 153, 157, 161}},
    {129, TD_EM_CHWIDTH160, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128}},
    {130, TD_EM_CHWIDTH80_80, TD_EM_CHAN_OFFSET_SCN,
          {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128,
           132, 136, 140, 144, 149, 153, 157, 161}},
    {0, 0, 0, {0}},
};

/*****************************************************************************
 函 数 名  : get_op_country_map
 功能描述  : 通过国家码找到操作类数组
 输入参数  : void *osif                            
             const td_em_op_class_map_t ** op_map  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
static int get_op_country_map(void *osif, const td_em_op_class_map_t ** op_map)
{
    if (!op_map || !osif) {
        TD_EM_DBG_PARAM_ERR("null pointer!\n");
        return -1;
    }

    switch (td_em_get_country_map(osif)) {
        case TD_EM_FCC: 
            *op_map = g_us_operating_class;
            TD_EM_DBG_MSG("select us_operating_class\n");
            break;
        case TD_EM_ETSI:
            *op_map = g_europe_operating_class;
            TD_EM_DBG_MSG("select europe_operating_class\n");
            break;
        case TD_EM_MKK:
            *op_map = g_japan_operating_class;
            TD_EM_DBG_MSG("select japan_operating_class\n");
            break;
        default://除了几个特殊国家之外，其他国家均选择全球码
            *op_map = g_global_operating_class;
            TD_EM_DBG_MSG("select global_operating_class\n");
            break;
    }

    return 0;
}

/*****************************************************************************
 函 数 名  : td_em_get_opclass
 功能描述  : 获取操作类 (暂不支持ax协议新增的131 132 133 134操作类)
 输入参数  : void *osif         
             char *data 
             unsigned char pow  
 输出参数  : 无
 返 回 值  : 当前芯片支持的操作类个数
 
 修改历史      :
  1.日    期   : 2020年5月19日
    作    者   : qinke
    修改内容   : 新生成函数

*****************************************************************************/
int td_em_get_opclass(void *osif, char *data, unsigned char pow)
{
    td_em_op_class_map_t *map = NULL;
    unsigned char idx = 0, total = 0;
    unsigned char cnt = 0, ch[MAX_CHANNELS_PER_OPERATING_CLASS] = {0};
    unsigned char cntop = 0, chop[MAX_CHANNELS_PER_OPERATING_CLASS] = {0};
    char newChanWidth;
    int maxch, minch;
    td_em_map_unoperable_ch_t *req = (td_em_map_unoperable_ch_t *)(data);
    td_em_bw_e channel_width = 0;

    if (!osif || !data) {
        TD_EM_DBG_PARAM_ERR("null ptr !\n");
        return -1;
    }

    get_op_country_map(osif, (const td_em_op_class_map_t **)&map);
    if (!map) {
        TD_EM_DBG_PARAM_ERR("get country map error!\n");
        return -1;
    }

    td_em_get_max_min_channel(osif, &maxch, &minch);
    channel_width = td_em_get_channel_width(osif);

    while (map->op_class) {
        while ((idx < MAX_CHANNELS_PER_OPERATING_CLASS) && map->ch_set[idx]) {
            if ((map->ch_set[0] > maxch) || ((TD_EM_CHWIDTH160 != channel_width) && (TD_EM_CHWIDTH160 == map->ch_width))
            || ((TD_EM_CHWIDTH80_80 != channel_width) && (TD_EM_CHWIDTH80_80 == map->ch_width))) {
                goto next;
            }

            if (TD_EM_CHWIDTH80 == map->ch_width || TD_EM_CHWIDTH80_80 == map->ch_width) {
                newChanWidth = 4;//计算频宽
            } else if (TD_EM_CHWIDTH160 == map->ch_width) {
                newChanWidth = 8;//计算频宽
            } else {
                newChanWidth = 1;//计算频宽
            }

            if ((idx + newChanWidth - 1) >= MAX_CHANNELS_PER_OPERATING_CLASS) {
                break;
            }

            if (map->ch_set[idx] && !(minch <= map->ch_set[idx] && map->ch_set[idx] <= maxch)) {
                ch[cnt] = (map->ch_set[idx] + map->ch_set[idx + newChanWidth - 1]) / 2;//计算中心频点

                cnt++;
                idx += newChanWidth;
            } else {
                chop[cntop] = (map->ch_set[idx] + map->ch_set[idx + newChanWidth - 1]) / 2;//计算中心频点
                cntop++;
                idx += newChanWidth;
            }
        }

        if (cnt != idx) {
            memcpy(req->unopch, ch, cnt * sizeof(ch[0]));
            memcpy(req->opch, chop, cntop * sizeof(chop[0]));
            req->num_opch = cntop;
            req->num_unopch = cnt;
            req->opclass = map->op_class;
            req->txpow = pow;
            req->band_width = map->ch_width;
            total++;
            req++;
        }

    next:
        map++;
        cnt = 0; 
        cntop = 0;
        idx = 0;
        memset(ch, 0, sizeof(ch));
        memset(chop, 0, sizeof(chop));
    }

    return total;
}
/*lint  +e19*/
/*lint  +e129*/
/*lint  +e10*/
/*lint  +e18*/