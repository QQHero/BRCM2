#ifndef LIB_DBAPI_DEBUG_H
#define LIB_DBAPI_DEBUG_H

#include <stdio.h>
#include <errno.h>
#include <string.h>

enum {
    DB_ERR_LEVEL = 1,
    DB_ERRNO_LEVEL,
    DB_DBG_LEVEL,
    DB_DETAIL_LEVEL,
    DB_WARN_LEVEL,
    DB_MAX_LEVEL
};

void db_loglevel_set(int level);
int db_log_printf(int level, char *format, ...);

#define DB_DBG_NAME "db"
#define DB_ERR(fmt, args...) db_log_printf(DB_ERR_LEVEL, "[%s]-->[%s]:[%d] [db err] "fmt"\n", __FILE__, __func__, __LINE__, ##args)
#define DB_DBG(fmt, args...) db_log_printf(DB_DBG_LEVEL, "[%s]-->[%s]:[%d] [db dbg] "fmt"\n", __FILE__, __func__, __LINE__, ##args)
#define DB_DETAIL(fmt, args...) db_log_printf(DB_DETAIL_LEVEL, "[%s]-->[%s]:[%d] [db detail] "fmt"\n", __FILE__, __func__, __LINE__, ##args)
#define DB_ERRNO(fmt, args...) DB_ERR("Failed(%d) to "fmt", %s.\n", errno, ##args, strerror(errno))
#define DB_WARN(fmt, args...) db_log_printf(DB_WARN_LEVEL, "[%s]-->[%s]:[%d] [db warn] "fmt"\n", __FILE__, __func__, __LINE__, ##args)

#endif //DB_DEBUG_H
