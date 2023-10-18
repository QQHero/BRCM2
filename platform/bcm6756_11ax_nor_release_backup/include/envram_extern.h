/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : envram_extern.h
  版 本 号   : 初稿
  作    者   : zzh
  生成日期   : 2016年3月26日
  最近修改   :
  功能描述   : 

  功能描述   : envram操作对外接口定义头文件

  修改历史   :
  1.日    期   : 2016年3月26日
    作    者   : zzh
    修改内容   : 创建文件

******************************************************************************/
#ifndef ENVRAM_EXTERN_H
#define ENVRAM_EXTERN_H

int envram_submit(void *unused);
int envram_get(int argc, char* argv[]);
int envram_set_value(char* name, char* value);
int envram_set(int argc, char* argv[]);
int envram_commit(int argc, char* argv[]);
int envram_show(void *unused);

#endif
