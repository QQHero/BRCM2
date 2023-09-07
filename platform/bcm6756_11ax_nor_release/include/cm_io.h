#pragma once

enum {
	CM_IO_CLOSED	= (1 << 1),
	CM_IO_CONNECTED = (1 << 2),
	CM_IO_CLOSING	= (1 << 3),
};

struct cm_io {
	struct cm_io *share; //TODO: so ugly, for concurrent reading and writing. refactor it later.
	ev_io io;
	ev_timer timer;
	void *cb;
	int nread;
	int nwrite;
	int err;
	int status;
	void *data; // public field for whatever data
};

static __inline int cm_io_is_active(struct cm_io *cmio)
{
	if (cmio->cb)
		return 1;
	cm_assert(!ev_is_active(&cmio->io));
	cm_assert(!ev_is_active(&cmio->timer));
	return 0;
}

static __inline int cm_io_is_closed(struct cm_io *cmio)
{
	if (cmio->status & CM_IO_CLOSED) {
		cm_assert(cmio->status == CM_IO_CLOSED);
		return 1;
	}
	return 0;
}

static __inline int cm_io_is_connected(struct cm_io *cmio)
{
	return cmio->status & CM_IO_CONNECTED;
}

static __inline int cm_io_is_closing(struct cm_io *cmio)
{
	return cmio->status & CM_IO_CLOSING;
}

static __inline int cm_io_get_fd(struct cm_io *cmio)
{
	if (cm_io_is_connected(cmio) && !cm_io_is_closing(cmio))
		return cmio->io.fd;
	return -1;
}

static __inline void cm_io_init(struct cm_io *cmio)
{
	memset(cmio, 0, sizeof(struct cm_io));
	cmio->io.fd = -1;
	cmio->status = CM_IO_CLOSED;
}

static __inline void cm_io_share(struct cm_io *cmio, struct cm_io *share)
{
	cm_assert(cmio != share);
	cm_assert(cmio->io.fd == -1);
	cm_assert(!cmio->share);
	cmio->share = share;
	cm_assert(share->io.fd == -1);
	cm_assert(!share->share);
	share->share = cmio;
}

typedef void (*cm_read_cb_t)(struct cm_io *cmio, int nread, sds data, int err);
void cm_io_read(struct cm_io *cmio, int nread, cm_read_cb_t cb, double timeout);

typedef void (*cm_write_cb_t)(struct cm_io *cmio, sds data, int nwrite, int err);
void cm_io_write(struct cm_io *cmio, sds data, cm_write_cb_t cb, double timeout);

typedef void (*cm_connect_cb_t)(struct cm_io *cmio, int err);
void cm_io_connect(struct cm_io *cmio, const char *addr, cm_connect_cb_t cb, double timeout);

typedef void (*cm_accept_cb_t)(struct cm_io *cmio, struct cm_io *newio, int err);
void cm_io_accept(struct cm_io *cmio, struct cm_io *newio, cm_accept_cb_t cb, double timeout);

int cm_io_listen(struct cm_io *cmio, const char *addr);

int cm_io_close(struct cm_io *cmio);

//NOTE:
// fd will be set to nonblocking mode.
// fd should be reopened before passed to this function if it refers to a tty (e.g. stdin).
int cm_io_set_fd(struct cm_io *cmio, int fd);

void cm_io_clear_fd(struct cm_io *cmio);
