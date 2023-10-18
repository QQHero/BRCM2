#pragma once

int cm_socket_error(int sock);
int cm_socket_connect(const char *addr, int *psock);
int cm_socket_listen(const char *addr, int *psock);
int cm_socket_accept(int sock, int *pnewsock);
int cm_socket_shutdown(int sock, int how);

#define CM_IP_MAX_LENGTH	64
int cm_socket_addr(int (*fn)(int, struct sockaddr *, socklen_t *),
				int sock, char *ip, uint16_t *port);

#define CM_READ		(1 << 0)
#define CM_WRITE	(1 << 1)
int cm_wait(int fd, int flags, double timeout);

/**
 * Read data with timeout
 *
 * bufsize > 0:  read exactly bufsize bytes
 * bufsize < 0:  read no more than -bufsize bytes
 * bufsize == 0: no effect
 *
 * Return the number of bytes read or -errno
 *
 * NOTE: fd should be set to nonblocking I/O mode when bufsize > 0
 */
int cm_read(int fd, char *buf, int bufsize, double timeout);

/**
 * Write data with timeout
 *
 * bufsize > 0:  write exactly bufsize bytes
 * bufsize == 0: no effect
 *
 * Return 0 or -errno
 *
 * NOTE: fd should be set to nonblocking I/O mode when bufsize > 0
 */
int cm_write(int fd, const char *buf, size_t bufsize, double timeout);
