//
// Created by penciljiang on 2022/1/10.
//

#ifndef START_ROUTER_ACCGW_ROUTER_LOG_H
#define START_ROUTER_ACCGW_ROUTER_LOG_H

#include <pthread.h>
#include<stdarg.h>
#include "router_time.h"

extern const int k_maxLogSize;
// trace time
extern const int s_traceTime;
// detail trace
extern const int s_traceOpen;
// trace file name
extern const int s_traceFile;
// 0 means trace verbose
extern const int s_logLevel;

enum LogLevel {
    VERBOSE__ = 0,
    DEBUG__ = 1,
    INFO__ = 2,
    WARNING__ = 3,
    ERROR__ = 4,
    DISABLE__ = 5,
};

pid_t GET_THREAD_ID();

inline static int appendLevelTag(enum LogLevel level, char *buf, int bufLen) {
  const char *tag = NULL;

  switch (level) {
    case VERBOSE__:
      tag = "[V]";
      break;
    case DEBUG__:
      tag = "[D]";
      break;
    case INFO__:
      tag = "[I]";
      break;
    case WARNING__:
      tag = "[W]";
      break;
    case ERROR__:
      tag = "[E]";
      break;
    case DISABLE__:
      return 0;
  }

  return snprintf(buf, bufLen, "%s ", tag);
}

inline static int appendLogTime(char *buf, int bufLen) {
  time_t tt = getSystemTimeMs() / 1000;
  struct tm *ttime;
  ttime = localtime(&tt);
  int len = strftime(buf, bufLen, "%H:%M:%S", ttime);
  int ms = getSystemTimeMs() % 1000;
  len += snprintf(buf + len, bufLen, ".%.3d ", ms);
  return len;
}

inline static int appendLogTimestamp(char *buf, int bufLen) {
  int64_t tt = getSystemTimeMs();
  int len = snprintf(buf, bufLen, "%lld ", tt);
  return len;
}

void PUSH_TO_LOG_QUEUE(enum LogLevel level, const char *fmt, ...);

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __PRINT_LOG__(level, fmt, ...) \
do { \
    if (s_logLevel > level) break; \
    if(s_traceOpen) { \
        PUSH_TO_LOG_QUEUE(level, "[%d][%s:%d(%s)] " fmt "\n", \
                GET_THREAD_ID(), __FILENAME__, __LINE__, __FUNCTION__, ## __VA_ARGS__); \
    }  \
    else if (s_traceFile) { \
        PUSH_TO_LOG_QUEUE(level, "[%s]" fmt "\n", __FUNCTION__, ## __VA_ARGS__); \
    } else {\
        PUSH_TO_LOG_QUEUE(level, fmt "\n", ## __VA_ARGS__); \
    }\
} while(0)

#define DEBUG_LOG_V(fmt, ...) \
    __PRINT_LOG__(VERBOSE__, fmt, ## __VA_ARGS__);

#define DEBUG_LOG_D(fmt, ...) \
    __PRINT_LOG__(DEBUG__, fmt, ## __VA_ARGS__);

#define DEBUG_LOG_I(fmt, ...) \
    __PRINT_LOG__(INFO__, fmt, ## __VA_ARGS__);

#define DEBUG_LOG_W(fmt, ...) \
    __PRINT_LOG__(WARNING__, fmt, ## __VA_ARGS__);

#define DEBUG_LOG_E(fmt, ...) \
    __PRINT_LOG__(ERROR__, fmt, ## __VA_ARGS__);



#endif  // START_ROUTER_ACCGW_ROUTER_LOG_H
