#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "sds.h"
#include "list.h"

#ifndef TEMP_FAILURE_RETRY
# define TEMP_FAILURE_RETRY(expression) \
   (__extension__                                                              \
     ({ long int __result;                                                     \
        do __result = (long int) (expression);                                 \
        while (__result == -1L && errno == EINTR);                             \
        __result; }))
#endif

#ifdef __cplusplus
extern "C" {
#endif

int mesh_htonl(int x);
int mesh_ntohl(int x);

#define cm_stringify(n)        cm_stringify_(n)
#define cm_stringify_(x)    #x

#define CM_FILE_LINE __FILE__ ":" cm_stringify(__LINE__)

#define DISABLE_CM_DBG_LEVEL    0 /* add by dengxingde, too many print,so close it */

#ifdef DISABLE_CM_DBG_LEVEL
#define cm_log(level, fmt, ...) do { \
    if (DISABLE_CM_DBG_LEVEL >= level) \
        fprintf(stderr, "[%.3f " CM_FILE_LINE " %s] " fmt "\r\n", cm_time(), __func__, ##__VA_ARGS__); \
} while (0)
#else

#define cm_log(level, fmt, ...) do { \
    if (CM_LOG_LEVEL >= level) \
        fprintf(stderr, "[%.3f " CM_FILE_LINE " %s] " fmt "\r\n", cm_time(), __func__, ##__VA_ARGS__); \
} while (0)
#endif

#define cm_debug(...)    cm_log(3, __VA_ARGS__)
#define cm_info(...)    cm_log(2, __VA_ARGS__)
#define cm_warn(...)    cm_log(1, __VA_ARGS__)
#define cm_error(...)    cm_log(0, __VA_ARGS__)

#define cm_assert(expr, ...) do { \
    if (!(expr)) \
        cm_abort("cm_assert(" #expr ") failure at <" CM_FILE_LINE ">\r\n" __VA_ARGS__); \
} while (0)

#define cm_check_oom(p) do { \
    if (!(p)) \
        cm_abort("oom at <" CM_FILE_LINE ">"); \
} while (0)

#define cm_new(type)        ((type *)cm_malloc(sizeof(type)))
#define cm_new_n(type, n)    ((type *)cm_malloc(sizeof(type) * (n)))
#define cm_new_v(type, elem, n) ((type *)cm_malloc(offsetof(type, elem[n])))

void cm_init();
int cm_run();
double cm_time();
void cm_abort(const char *fmt, ...);

static __inline void *cm_malloc(size_t size)
{
    void *p;

    p = malloc(size);
    cm_check_oom(p);
    return p;
}

static __inline void cm_free(void *p)
{
    free(p);
}

static __inline int cm_fd_set_nonblock(int fd, int set)
{
    return TEMP_FAILURE_RETRY(ioctl(fd, FIONBIO, &set)) == 0 ? 0 : -errno;
}

static __inline int cm_fd_set_cloexec(int fd, int set)
{
    return TEMP_FAILURE_RETRY(ioctl(fd, set ? FIOCLEX : FIONCLEX)) == 0 ? 0 : -errno;
}

static __inline int cm_fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

#include "cm_errno.h"
#include "cm_socket.h"
#include "cm_ev.h"
#include "cm_io.h"
#include "cm_timer.h"
#include "cm_poll.h"
#include "cm_process.h"
#include "cm_signal.h"

#ifdef __cplusplus
}
#endif