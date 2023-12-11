#pragma once

#include "hiredis.h"

#ifdef __cplusplus
extern "C" {
#endif

struct redis_reader;

typedef int (*redis_reader_cb)(struct redis_reader *r, void *obj);

struct redis_reader {
	redis_reader_cb cb;
	int encap;
	int state;
	int pos;
	int bodylen;
	sds buf;
	redisReader raw;
};

void redis_reader_init(struct redis_reader *r, redis_reader_cb cb, int encap);
void redis_reader_cleanup(struct redis_reader *r);
void redis_reader_reset(struct redis_reader *r);
int redis_reader_feed(struct redis_reader *r, const char *buf, size_t size);


struct redis_writer {
	int encap;
	int buflimit;
	sds buf;
};

void redis_writer_init(struct redis_writer *w, int buflimit, int encap);
void redis_writer_cleanup(struct redis_writer *w);
void redis_writer_reset(struct redis_writer *w);
int redis_writer_push(struct redis_writer *w, sds obj);
sds redis_writer_flush(struct redis_writer *w);

void redis_encap_header_dump(void *p);

#ifdef __cplusplus
}
#endif
