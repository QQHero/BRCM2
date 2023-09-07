#pragma once

#include "redis_client.h"

#ifdef __cplusplus
extern "C" {
#endif

void cmd_init(struct redis_option *option);

void cmd_on_connect(void (*cb)(int err));
void cmd_on_close(void (*cb)(int err));

// value is guaranteed to be null terminated
typedef void (*cmd_sub_cb)(const char *key, const char *value, size_t size, void *data);
void *cmd_sub(const char *key, cmd_sub_cb cb, void *data);

// watcher: the pointer return from cmd_sub
void cmd_unsub(void *watcher);

int cmd_pub(const char *key, const char *value, size_t size);

int cmd_set(const char *key, const char *value, size_t size);

// *pvalue is guaranteed to be null terminated and should be freed by caller
int cmd_get(const char *key, char **pvalue, size_t *psize);

#ifdef __cplusplus
}
#endif
