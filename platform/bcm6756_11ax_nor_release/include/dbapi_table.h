/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : table.h
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : declaration of attributes and operations of sqlite3 table
  History       :

******************************************************************************/
#ifndef LIB_DBAPI_TABLE_H
#define LIB_DBAPI_TABLE_H

#include "dbapi_backup.h"
#include "dbapi_column.h"
#include "dbapi_result.h"

typedef struct db_table
{
    char *name; // 名字
    char *db_name; // 数据库名, NULL则使用默认
    db_backup_t *backup; // 数据库备份对象
    db_col_t *cols; // 列
    unsigned char col_num; // 列数
    unsigned char inited: 1; // 是(1)否(0)已初始化

    /*
     *  lock - 加锁
     *  @self: 表
     *
     *  加锁并开启事务.
     *  NODE: 加锁(事务)可以加快多条数据库"写"操作的执行, 实际只在解锁时写一次
     *  ----例如, 一次性删除5条静态IP绑定规则, 加锁执行耗时不到1秒, 无锁则需3秒左右
     *  NOTE: 这里是对数据库加锁, 如果相同数据库的多个表执行加锁, 实际只加一个锁
     *      若只执行一个动作/命令(如add/del/set/find/all/...), 不需要加锁!
     */
    void (*lock)(struct db_table *self);

    /*
     *  unlock - 解锁
     *  @self: 表
     *
     *  解锁并关闭事务.
     *  NOTE: 这里是对数据库解锁, 如果相同数据库的多个表执行解锁, 实际只解一个锁
     */
    void (*unlock)(struct db_table *self);

    /*
     *  rollback - 回滚
     *  @self: 表
     *
     *  回滚事务
     */
    void (*rollback)(struct db_table *self);

    /*
     *  add - 添加一行
     *  @self: 表
     *  @values: 各个列的值
     *
     *  值的数量必须与初始化表时(db_table_init)指定的列数一致
     *
     *  return: 失败返回0, 成功返回新增的行的ID
     */
    int (*add)(struct db_table *self, db_val_t values[]);
    /*
     *  insert - 找到空的id位置插入一行
     *  @self: 表
     *  @values: 各个列的值
     *
     *  值的数量必须与初始化表时(db_table_init)指定的列数一致
     *
     *  return: 失败返回0, 成功返回新增的行的ID
     */
    int (*insert)(struct db_table *self, db_val_t values[]);

    /*
     *  add_multi - 添加多行
     *  @self: 表
     *  @replace: 冲突时是(1)否(0)覆盖
     *  @values: 各个列的值
     *  @n: 要添加的行数
     *
     *  该函数比开启事务后添加多行的方式更快
     *
     *  return: 1-ok, o-fail
     */
    int (*add_multi)(struct db_table *self, int replace, db_val_t values[], unsigned n);

    /*
     *  add_or_replace - 添加一行, 若存在则覆盖
     *  @self: 表
     *  @values: 各个列的值
     *
     *  值的数量必须与初始化表时(db_table_init)指定的列数一致
     *
     *  return: 1-ok, o-fail
     */
    int (*add_or_replace)(struct db_table *self, db_val_t values[]);

    /*
     *  add_or_ignore - 添加一行, 若存在则忽略
     *  @self: 表
     *  @values: 各个列的值
     *
     *  值的数量必须与初始化表时(db_table_init)指定的列数一致
     *
     *  return: 1-ok, o-fail
     */
    int (*add_or_ignore)(struct db_table *self, db_val_t values[]);

    /*
     *  get - 根据ID查找行
     *  @self: 表
     *  @id: 要查找的行的ID
     *
     *  返回的行在使用之后注意要释放(free)
     *
     *  return: 查找到的行, 是否返回NULL
     */
    db_row_ptr_t (*get)(struct db_table *self, int id);

    /*
     *  set - 根据ID修改行
     *  @self: 表
     *  @id: 要修改的行的ID
     *  @values: 各个列的值
     *
     *  return: 1-ok, 0-fail
     */
    int (*set)(struct db_table *self, int id, db_val_t values[]);

    /*
     *  set - 根据ID和列序号修改列
     *  @self: 表
     *  @id: 要修改的行的ID
     *  @col_index: 列序号
     *  @col_val: 列的新值
     *
     *  return: 1-ok, 0-fail
     */
    int (*set_col)(struct db_table *self, int id, int col_index, db_val_t col_val);

    /*
     *  set_by - 根据列值修改行
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *  @values: 各个列的值
     *
     *  return: 1-ok, 0-fail
     */
    int (*set_by)(struct db_table *self, int col_index, db_val_t col_val, db_val_t values[]);

    /*
     *  update_row - 更新指定的行(只更新修改了的列值)
     *  @self: 表
     *  @row: 要修改的行
     *
     *  return: 1-ok, 0-fail
     */
    int (*update_row)(struct db_table *self, db_row_t *row);

    /*
     *  del - 根据ID删除行
     *  @self: 表
     *  @id: 要删除的行的ID
     *
     *  return: 1-ok, 0-fail
     */
    int (*del)(struct db_table *self, int id);

    /*
     *  del_by - 根据列值删除行
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *
     *  return: 1-ok, 0-fail
     */
    int (*del_by)(struct db_table *self, int col_index, db_val_t col_val);

    /*
     *  find - 根据列值查找行
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *
     *  仅返回找到的第一行
     *  返回的行在使用之后注意要释放(free)
     *
     *  return: 查找到的行, 是否返回NULL
     */
    db_row_ptr_t (*find)(struct db_table *self, int col_index, db_val_t col_val);

    /*
     *  find_all - 根据列值查找行
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *  @result: 查询的结果
     *
     *  把所有找到的行, 放在result中,
     *  使用完后注意释放内存: result.free(&result)
     *
     *  return: 1-ok, 0-fail
     */
    int (*find_all)(struct db_table *self, int col_index, db_val_t col_val, db_result_t *result);

    /*
     *  all - 获取全部记录(所有行)
     *  @self: 表
     *  @result: 查询的结果
     *
     *  把所有找到的行, 放在result中,
     *  使用完后注意释放内存: result.free(&result)
     *
     *  return: 1-ok, 0-fail
     */
    int (*all)(struct db_table *self, db_result_t *result);

    /*
     *  clear - 删除全部记录(清空表)
     *  @self: 表
     *
     *  return: 1-ok, 0-fail
     */
    int (*clear)(struct db_table *self);
        /*
     *  clear_seq - 清空表的自增id重新计算
     *  @self: 表
     *
     *  return: 1-ok, 0-fail
     */
    int (*clear_seq)(struct db_table *self);

    /*
     *  count - 统计行数
     *  @self: 表
     *
     *  return: 行数
     */
    int (*count)(struct db_table *self);

    /*
     *  count - 统计列值匹配的行数
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *
     *  统计符合条件 (第@col_index列的值为@col_val) 的行数
     *
     *  return: 行数
     */
    int (*count_by)(struct db_table *self, int col_index, db_val_t col_val);

    /*
     *  inc_refer - 引用计数+1
     *  @self: 表
     *  @id: 要操作的行的ID
     *
     *  return: 1-ok, 0-fail
     */
    int (*inc_refer)(struct db_table *self, int id);

    /*
     *  dec_refer - 引用计数-1
     *  @self: 表
     *  @id: 要操作的行的ID
     *
     *  return: 1-ok, 0-fail
     */
    int (*dec_refer)(struct db_table *self, int id);

    /*
     *  clr_refer - 引用计数=0
     *  @self: 表
     *  @id: 要操作的行的ID
     *
     *  return: 1-ok, 0-fail
     */
    int (*clr_refer)(struct db_table *self, int id);

    /*
     *  select - 高级查询, 可以定制查询的详细参数
     *  @self: 表
     *  @condition: 查询条件 (SQL语句中的 WHERE 子句), 为空表示不筛选
     *  @order_by: 要排序的列的名称 (SQL语句中 ORDER BY 子句), 为空表示不排序
     *  @offset: 忽略查询结果中 第 @offset 行 之前的行
     *  @limit: 限制查询结果的行数, 忽略多余的行, 0表示不限制
     *  @result: 查询的结果
     *
     *  多列排序优先写在前面的列,
     *  比如order_by="name,age,score" 先按name排序,然后再按age, 最后按score
     *
     *  用 @offset 和 @limit 可以对查询结果切片, 从第 @offset 行开始, 共 @limit 行
     *
     *  把所有找到的行, 放在result中,
     *  使用完后注意释放内存: result.free(&result)
     *
     *  示例:
     *  table->select(table, "id >= 3 AND id <= 9", "name,age DESC,score", 0, 0, &result);
     *  ==> SELECT * FROM XXX WHERE (id >= 3 AND id <= 9) ORDER BY name,age DESC,score LIMIT -1 OFFSET 0;
     *
     *  return: 1-ok, 0-fail
     */
    int (*select)(struct db_table *self,
                  char *condition, char *order_by, int offset, int limit,
                  db_result_t *result);

    /*
     *  update - 高级设置, 可以定制匹配的条件
     *  @self: 表
     *  @setcmd: 要修改的列和值 (SQL语句中的SET子句)
     *  @condition: 匹配条件 (SQL语句中的WHERE子句), 空则表示对所有行都设置
     *
     *  示例:
     *  table->update(table, "a='a',b='bbb'", "id >= 3 AND id <= 9");
     *  ==> UPDATE XXX SET a='a',b='bbb' WHERE (id >= 3 AND id <= 9);
     *
     *  return: 1-ok, 0-fail
     */
    int (*update)(struct db_table *self, char *setcmd, char *condition, ...);

    /*
     *  delete - 高级删除, 可以定制匹配的条件
     *  @self: 表
     *  @condition: 匹配条件 (SQL语句中的WHERE子句), 空则表示所有行
     *
     *  示例:
     *  table->delete(table, "id >= 3 AND id <= 9");
     *  ==> DELETE FROM XXX WHERE (id >= 3 AND id <= 9);
     *
     *  return: 1-ok, 0-fail
     */
    int (*delete)(struct db_table *self, char *condition, ...);

} db_table_t;

/*
 *  db_table_init - 初始化表结构
 *  @db_name: 数据库
 *  @table: 表结构体指针
 *  @tab_name: 表的名字
 *  @cols: 列结构体数组
 *  @col_num: 列的数量
 *
 *  returns: 1-ok, 0-fail
 */
int db_table_init(char *db_name, db_table_t *table, char *tab_name,
                  db_col_t cols[], int col_num);

/*****************************************************************************
 函 数 名  : db_table_set_delay_sync_memdb
 功能描述  : 设置延时同步标志
 输入参数  : db_table_t *self  
             int delay_sync    
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2021年9月13日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
void db_table_set_delay_sync_memdb(db_table_t *self, int delay_sync);

/*****************************************************************************
 函 数 名  : db_table_delay_sync_memdb_exec
 功能描述  : 延时同步（延时从内存数据库同步到falsh数据
                 库）处理函数，需要调用这自行调用
 输入参数  : db_table_t *self      
             char* path_main_db    
             char* path_backup_db  
 输出参数  : 无
 返 回 值  : 
 
 修改历史      :
  1.日    期   : 2021年9月13日
    作    者   : xiaotiantian
    修改内容   : 新生成函数

*****************************************************************************/
int db_table_delay_sync_memdb_exec(db_table_t *self, char* path_main_db, char* path_backup_db);

#endif // LIB_DBAPI_TABLE_H