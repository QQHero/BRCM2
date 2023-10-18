#ifndef LIB_DBAPI_BACKUP_H
#define LIB_DBAPI_BACKUP_H

#include <pthread.h>
#include "dbapi_database.h"

#define DB_DUMP_START       "###> product database [ %s ] start >###"
#define DB_DUMP_END         "###< product database [ %s ] end <###"

typedef sqlite_uint64 uint64;         /* 8-byte unsigned integer */

typedef struct db_backup
{
    struct list_head link;
    struct list_head sync;
    database_t main;   // 主数据库
    database_t backup; // 备份数据库
    database_t mem;    // 内存数据库
    unsigned char inited: 1;
    unsigned char lock_cnt;
    unsigned short sync_cnt;
    pthread_mutex_t mutex;
    uint64 lock_start;
    unsigned char delay_sync;//延时同步内存数据库到主数据库的时间,0表示不采用这种延时机制

    /*
     *  modify - 执行指定SQL修改语句
     *  @self: 数据库备份对象
     *  @cmd: 要执行的语句
     *  @result: 保存执行结果, 若NULL则不保存结果
     *
     *  result使用完后注意释放内存: result.free(&result)
     *
     *  return: SQLITE_OK(0) if ok, else return SQLITE_XXX error code
     */
    int (*modify)(struct db_backup *self, char *cmd, db_result_t *result);

    /*
     *  query - 执行指定SQL查询语句
     *  @self: 数据库备份对象
     *  @cmd: 要执行的语句
     *  @result: 保存执行结果, 若NULL则不保存结果
     *
     *  result使用完后注意释放内存: result.free(&result)
     *
     *  return: SQLITE_OK(0) if ok, else return SQLITE_XXX error code
     */
    int (*query)(struct db_backup *self, char *cmd, db_result_t *result);

    /*
     *  rollback - 回滚
     *  @self: 数据库备份对象
     *
     *  回滚数据库事务
     */
    void (*rollback)(struct db_backup *self);

    /*
     *  recover - 尝试从异常中恢复
     *  @self: 数据库备份对象
     *  @main_err: 主数据库错误码
     *  @backup_err: 备份数据库错误码
     *  @mem_err: 内存数据库错误码
     */
    void (*recover)(struct db_backup *self, int main_err, int backup_err, int mem_err);

    /*
     *  lock - 加锁
     *  @self: 数据库备份对象
     *
     *  申请互斥锁, 若锁已被占用, 则挂起等待, 直到占有者主动释放锁
     *  处理完后注意要释放锁( self->unlock )
     *
     *  这个lock函数是保证多条语句执行过程中不会被其他人修改,
     *  即: 执行单条数据库语句是不需要lock的
     */
    void (*lock)(struct db_backup *self);

    /*
     *  unlock - 解锁
     *  @self: 数据库备份对象
     *
     *  释放互斥锁
     *  与 self->lock 配合使用
     */
    void (*unlock)(struct db_backup *self);

    /*
     *  delay_sync_func - 执行数据库备份
     *  @self: 数据库备份对象
     *  @path_backup_db 备份数据库的路劲
     *
     */
    int (*delay_sync_func)(struct db_backup *self, char* path_backup_db);

} db_backup_t;

/*
 *  db_backup_save - 备份数据库到文件
 *  @name: 数据库名字
 *  @filename: 文件名
 *
 *  把数据库内容dump到指定文件中
 */
void db_backup_save(char *db_name, char *filename);

/*
 *  db_backup_load - 恢复数据库
 *  @name: 数据库名字
 *  @data: 数据库内容(SQL语句集)
 *
 *  在指定数据库执行 @data 语句集
 */
void db_backup_load(char *db_name, char *data);

db_backup_t *get_backup_obj(char *name);
int db_get_col_name_list(db_backup_t *self, char *tbl_name, db_result_t *result);

#endif // LIB_DBAPI_BACKUP_H