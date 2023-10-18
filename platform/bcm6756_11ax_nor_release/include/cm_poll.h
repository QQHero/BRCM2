#pragma once

struct cm_poll;

typedef void (*cm_poll_cb)(struct cm_poll *cp, int fd, int events);

struct cm_poll {
	ev_io io;
	cm_poll_cb cb;
};

static __inline void cm_poll_init(struct cm_poll *cp)
{
	memset(cp, 0, sizeof(struct cm_poll));
}

// events: CM_READ or CM_WRITE or CM_READ | CM_WRITE
void cm_poll_start(struct cm_poll *cp, int fd, int events, cm_poll_cb cb);
void cm_poll_stop(struct cm_poll *cp);
