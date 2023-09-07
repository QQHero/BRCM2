//
// Created by penciljiang on 2022/1/10.
//

#ifndef START_ROUTER_ACCGW_ROUTER_TIME_H
#define START_ROUTER_ACCGW_ROUTER_TIME_H

#include <sys/time.h>
#include <stdint.h>

static int64_t getSystemTimeMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t value = ((int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000);
  return value;
}

#endif  // START_ROUTER_ACCGW_ROUTER_TIME_H
