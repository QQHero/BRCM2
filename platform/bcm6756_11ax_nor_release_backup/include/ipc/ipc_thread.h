/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : thread.h
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-10-10
  Last Modified :
  Description   : thread wrapper
  Function List :
  History       :

******************************************************************************/
#ifndef LIB_DBAPI_IPC_THREAD_H
#define LIB_DBAPI_IPC_THREAD_H

#include <pthread.h>

typedef void (*db_thread_body_t)(void *args);

/*
 * db_start_thread - 创建并启动线程
 * @body: 线程体函数
 * @args: 原样传给 @body 函数
 *
 * 注意args最好不要用指向局部变量的指针,
 * 因为在 @body 中访问时该局部变量可能已被释放
 *
 * return: 线程id, 失败返回负数
 */
pthread_t db_start_thread(db_thread_body_t body, void *args);

/*
 * db_set_thread_name - 设置当前线程的名字
 * @fmt: just like printf
 * @...: just like printf
 *
 * return: 0-fail, 1-ok
 */
int db_set_thread_name(const char *fmt, ...);

char *db_get_thread_name(void);

#endif // LIB_DBAPI_IPC_THREAD_H