#ifndef LIB_DBAPI_IPC_SHARED_MEMORY_H
#define LIB_DBAPI_IPC_SHARED_MEMORY_H

/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : shared_memory.h
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-9-29
  Last Modified :
  Description   : shared memory between processes
  Function List :
  History       :

******************************************************************************/

/*
 * db_get_shared_memory - 获取共享内存
 * @name: 要操作的共享内存的唯一名字
 * @size: 内存的大小
 *
 * 如果不存在则创建
 *
 * return: 成功返回指针, 失败返回NULL
 */
void *db_get_shared_memory(char *name, unsigned size);

#endif // LIB_DBAPI_IPC_SHARED_MEMORY_H