#pragma once

#include "redis_client.h"

#ifdef __cplusplus
extern "C" {
#endif

struct subscribe_client;
struct subscribe_watcher;
/*使用全局变量记录当前设备的角色*/
extern char g_redis_so_irole;

/* NOTE:
 *  1. value is read only.
 *  2. It will be called with a NULL value when subscription is ready.
 *  3. subscribe_register() can not be called in cb.
 */
typedef void (*subscribe_watcher_cb)(struct subscribe_watcher *watcher, sds value);

struct subscribe_watcher {
	struct list_head node;		// link to watchers of struct subscribe_entry, private field
	subscribe_watcher_cb cb;	// private field
	sds key;					// read only to user
};

struct subscribe_client *subscribe_new(struct redis_option *option);

void subscribe_register(struct subscribe_client *cli,
						struct subscribe_watcher *watcher,
						const char *key,
						subscribe_watcher_cb cb);

void subscribe_unregister(struct subscribe_client *cli,
						struct subscribe_watcher *watcher);

char reids_so_get_g_redis_so_irole(void);
void reids_so_set_g_redis_so_irole(char flag);

#ifdef __cplusplus
}
#endif
