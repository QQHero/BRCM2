#ifndef LIB_DBAPI_TOOL_H
#define LIB_DBAPI_TOOL_H

#include <stdlib.h>
#include <stdint.h>


/*
    DB_STR_APPEND - append string to a string buffer
    @buf: buffer, char array
    @buf_sz: size of @buf
    @fmt: format like printf
    @.....: arguments like printf, such as %d=int, %f=float ...

    need not care about the return value
*/
#define DB_STR_APPEND(buf, buf_sz, fmt, ...) \
    snprintf((buf) + strlen(buf), (buf_sz) - strlen(buf), fmt, ##__VA_ARGS__)

/*
    DB_FREE_IF_NOT_NULL - 安全释放内存
    @ptr: 内存指针
*/
#define DB_FREE_IF_NOT_NULL(ptr) if (ptr) {\
    free(ptr); \
}

/* DB_MACRO_CONCAT - 把两个标识符连接起来 */
#define DB_MACRO_CONCAT(a, b) a##b
/* DB_MID_VAR - 中间宏,不要直接调用 */
#define DB_MID_VAR(prefix, x) DB_MACRO_CONCAT(prefix, x)
/* DB_LINE_VAR - 生成一个包含行号的标识符, 比如 DB_LINE_VAR(hello)将被替换成 _hello39 (39是调用宏的行号) */
#define DB_LINE_VAR(name) DB_MID_VAR(DB_MACRO_CONCAT(_, name), __LINE__)
/* DB_FUNC_VAR - 生成一个包含函数名的标识符, 比如 DB_FUNC_VAR(hello)将被替换成 _hello_main */
#define DB_FUNC_VAR(name) DB_MID_VAR(DB_MACRO_CONCAT(_, name), DB_MACRO_CONCAT(_, __func__))

/*
    DB_STR_STARTS_WITH - 判断字符串是否以 某前缀 开头
    @str: 要判断的字符串
    @prefix: 前缀

    returns: 1-是, 0-否
*/
#define DB_STR_STARTS_WITH(str, prefix) (0 == strncmp(str, prefix, strlen(prefix)))

int db_exec_sys_cmd(const char *func, int line, char *cmd, ...);
#define db_api_exec_cmd(...) db_exec_sys_cmd(__func__, __LINE__, __VA_ARGS__)

int db_get_sys_uptime();
char *db_get_self_pname(void);
char *db_str_replace(char *str, char *old, char *new);
void db_do_file_cmd(char *filename,int opt,char *fmt, ...);
void db_replace_char(char *str, char old_char, char new_char);
uint64_t db_get_now_ms();
void db_touch_file(char *filename);



#endif
