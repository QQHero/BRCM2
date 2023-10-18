/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : column.h
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : declaration of attributes and operations of sqlite3 column
  History       :

******************************************************************************/
#ifndef LIB_DBAPI_KEYVALUE_H
#define LIB_DBAPI_KEYVALUE_H

#include "dbapi_table.h"

#define KV_DB_NAME "kv"
#define KV_TABLE_NAME "kv"
#define KV_FIXED_DB_NAME DB_PATH"/kv.fixed" /* 使用绝对路径以避免被复位操作删除 */
#define KV_TMP_DB_NAME "/var/kv.db"

#define DB_MAX_KEY_LEN  500
#define DB_MAX_VAL_LEN  1500

/*
 * 以下是提供给外部的接口
 */

/*
    lock_value/unlock_value:
    lock_fixed_value/unlock_fixed_value:
    lock_tmp_value/unlock_tmp_value:
    批量 set 时, 使用 lock/unlock (数据库事务) 能加快速度

    *tmp* 是临时配置(重启系统后消失)
    *fixed* 是默认配置(复位后仍然存在)
*/
#define lock_value() db_lock_value(get_kv_table())
#define unlock_value() db_unlock_value(get_kv_table())
#define lock_tmp_value() db_lock_value(get_tmp_kv_table())
#define unlock_tmp_value() db_unlock_value(get_tmp_kv_table())
#define lock_fixed_value() db_lock_value(get_fixed_kv_table())
#define unlock_fixed_value() db_unlock_value(get_fixed_kv_table())

/*
    set_value - 设置配置值
    @key: 配置键
    @value: 配置值
    @args: printf-like, @key 的格式化参数

    *tmp* 是临时配置(重启系统后消失)
    *fixed* 是默认配置(复位后仍然存在)

    return: 1=成功, 0=失败
*/
#define set_value(key, value, args...) db_set_value(get_kv_table(), value, key,  ##args)
#define set_tmp_value(key, value, args...) db_set_value(get_tmp_kv_table(), value, key,  ##args)
#define set_fixed_value(key, value, args...) db_set_value(get_fixed_kv_table(), value, key,  ##args)

/*
    set_int_value - 设置配置值(整数)
    @key: 配置键
    @value: 配置值
    @args: printf-like, @key 的格式化参数

    *tmp* 是临时配置(重启系统后消失)
    *fixed* 是默认配置(复位后仍然存在)

    return: 1=成功, 0=失败
*/
#define set_int_value(key, value, args...) db_set_int_value(get_kv_table(), (int)(value), (char *)(key),  ##args)
#define set_tmp_int_value(key, value, args...) db_set_int_value(get_tmp_kv_table(), value, key,  ##args)
#define set_fixed_int_value(key, value, args...) db_set_int_value(get_fixed_kv_table(), value, key,  ##args)

/*
    get_value - 获取配置值
    @key: 配置键
    @value: 配置值
    @args: printf-like, @key 的格式化参数

    根据 @key 查找配置, 把找到的配置值写入 @value 中

    *tmp* 是临时配置(重启系统后消失)
    *fixed* 是默认配置(复位后仍然存在)

    return: 1=成功, 0=失败
*/
#define get_value(key, value, value_size, args...) db_get_value(get_kv_table(), value, value_size, key, ##args)
#define get_tmp_value(key, value, value_size, args...) db_get_value(get_tmp_kv_table(), value, value_size, key, ##args)
#define get_fixed_value(key, value, value_size, args...) db_get_value(get_fixed_kv_table(), value, value_size, key, ##args)

/*
    get_value - 获取配置值
    @key: 配置键
    @value: 配置值
    @args: printf-like, @key 的格式化参数

    根据 @key 查找配置, 把找到的配置值写入 @value 中

    *tmp* 是临时配置(重启系统后消失)
    *fixed* 是默认配置(复位后仍然存在)

    return: 1=成功, 0=失败
*/
#define get_int_value(key, args...) db_get_int_value(get_kv_table(), key, ##args)
#define get_tmp_int_value(key, args...) db_get_int_value(get_tmp_kv_table(), key, ##args)
#define get_fixed_int_value(key, args...) db_get_int_value(get_fixed_kv_table(), key, ##args)

/*
    del_value - 删除配置值
    @key: 配置键
    @args: printf-like, @key 的格式化参数

    *tmp* 是临时配置(重启系统后消失)
    *fixed* 是默认配置(复位后仍然存在)

    return: 1=成功, 0=失败
*/
#define del_value(key, args...) db_del_value(get_kv_table(), key, ##args)
#define del_tmp_value(key, args...) db_del_value(get_tmp_kv_table(), key, ##args)
#define del_fixed_value(key, args...) db_del_value(get_fixed_kv_table(), key, ##args)


/*>>> [begin: internal declare] 以下函数, 外部不要直接调用 */
db_table_t *get_kv_table(void);
db_table_t *get_fixed_kv_table(void);
db_table_t *get_tmp_kv_table(void);
int db_lock_value(db_table_t *table);
int db_unlock_value(db_table_t *table);
int db_del_value(db_table_t *table, char *key, ...);
int db_set_value(db_table_t *table, char *key, char *value, ...);
int db_get_value(db_table_t *table, char *value, int value_size, char *key, ...);
int db_set_int_value(db_table_t *table, int value, char *key, ...);
int db_get_int_value(db_table_t *table, char *key, ...);
/*<<< [end: internal declare] 以上函数, 外部不要直接调用 */
#endif // LIB_DBAPI_KEYVALUE_H