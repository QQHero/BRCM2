/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : nvram_extern.h
  版 本 号   : 初稿
  作    者   : zzh
  生成日期   : 2016年3月26日
  最近修改   :
  功能描述   : 

  功能描述   : nvram对外操作接口头文件

  修改历史   :
  1.日    期   : 2016年3月26日
    作    者   : zzh
    修改内容   : 创建文件

******************************************************************************/
#ifndef NVRAM_EXTERN_H
#define NVRAM_EXTERN_H

char *bcm_nvram_get(const char *name);
int bcm_nvram_match(char *name, char *match);
int bcm_nvram_getall(char *buf, int count);
int bcm_nvram_set(const char *name, const char *value);
int bcm_nvram_unset(const char *name);
int bcm_nvram_commit(void);

#define bcm_nvram_safe_get(name)  (bcm_nvram_get(name) ? : "")
#endif

