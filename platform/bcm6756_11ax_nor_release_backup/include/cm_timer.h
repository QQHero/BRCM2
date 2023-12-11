#pragma once

struct cm_timer {
	ev_timer ev;
	void *cb;
};

static __inline void cm_timer_init(struct cm_timer *timer)
{
	memset(timer, 0, sizeof(struct cm_timer));
}

typedef void (*cm_timer_cb_t)(struct cm_timer *timer);
void cm_timer_start(struct cm_timer *timer, double interval, cm_timer_cb_t cb);
void cm_timer_stop(struct cm_timer *timer);

typedef void (*cm_timeout_cb_t)(void *data);
void cm_timeout(double timeout, cm_timeout_cb_t cb, void *data);
