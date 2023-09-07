#pragma once

struct cm_signal;

typedef void (*cm_signal_cb)(struct cm_signal *cs, int signum);

struct cm_signal {
	ev_signal ev;
	cm_signal_cb cb;
};

static __inline void cm_signal_init(struct cm_signal *cs)
{
	memset(cs, 0, sizeof(struct cm_signal));
}

void cm_signal_start(struct cm_signal *cs, int signum, cm_signal_cb cb);
void cm_signal_stop(struct cm_signal *cs);

typedef void (*cm_signal_watch_cb)(int signum, void *data);
void cm_signal_watch(int signum, cm_signal_watch_cb cb, void *data);
