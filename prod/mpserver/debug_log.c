#include <stdio.h>
#include "router_log.h"

const int k_maxLogSize = 512;
// trace time
const int s_traceTime = 1;
// detail trace
const int s_traceOpen = 1;
// trace file name
const int s_traceFile = 1;
// 0 means trace verbose
const int s_logLevel = 0;

pid_t GET_THREAD_ID() {
  return pthread_self();
}


void PUSH_TO_LOG_QUEUE(enum LogLevel level, const char *fmt, ...) {
  // k_maxLogSize = 512
  char line[512] = {0};

  int offset = 0;
  if (s_traceTime) {
    offset += appendLogTime(line + offset, k_maxLogSize - offset);
  }
  offset += appendLevelTag(level, line + offset, k_maxLogSize - offset);
  if (s_traceTime) {
    offset += appendLogTimestamp(line + offset, k_maxLogSize - offset);
  }

  int vs_read = 0;
  va_list args;
  va_start(args, fmt);
  vs_read = vsnprintf(line + offset, k_maxLogSize - offset, fmt, args);
  va_end(args);

  if (vs_read > k_maxLogSize) {
    offset += k_maxLogSize - offset;
    line[offset - 4] = '.';
    line[offset - 3] = '.';
    line[offset - 2] = '\n';
    line[offset - 1] = '\0';
  } else {
    offset += vs_read;
  }
  // TODO output to local files..
  printf("%s", line);
}