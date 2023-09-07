#pragma once

#include "cm.h"
#include "redis_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

struct redis_client;
struct redis_option;
struct redis_task;

typedef void (*redis_task_cb)(struct redis_task *task, sds reply, int err);

struct redis_task {
	struct list_head node;	// link to pending_tasks of redis_client
	redis_task_cb cb;
};

struct redis_option {
	const char *server_address;
	int message_encap;
	double reconnect_interval;
	double connect_timeout;
	double send_timeout;
	double recv_timeout;
	void (*on_connect)(struct redis_client *redis, int err);
	void (*on_close)(struct redis_client *redis, int err);
	void (*on_drain)(struct redis_client *redis);
};

struct redis_client {
	struct cm_io rio;
	struct cm_io wio;
	struct cm_timer timer;
	struct list_head pending_tasks;	// list of redis_task
	struct redis_reader reader;
	struct redis_writer writer;
	struct redis_option *option;
};

static __inline void redis_task_init(struct redis_task *task, redis_task_cb cb)
{
	cm_assert(cb);
	task->cb = cb;
}

struct redis_option *redis_option_new();
void redis_client_init(struct redis_client *redis, struct redis_option *option);
int redis_client_execute(struct redis_client *redis, struct redis_task *task, sds req);
int redis_client_try_execute(struct redis_client *redis, struct redis_task *task, sds req);

#ifdef __cplusplus
}
#endif
