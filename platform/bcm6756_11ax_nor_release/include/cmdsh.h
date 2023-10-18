#pragma once

#include "sds.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMDSH_REQUEST		0xB96DCA00
#define CMDSH_REPLY			0x8FE39B00
#define CMDSH_FLAGS_MASK	0xFF
#define CMDSH_FLAGS_DESC	"FRNCWIOE"

enum {
	CMDSH_FIN = 1 << 0,
	CMDSH_RST = 1 << 1,
	CMDSH_NEW = 1 << 2,
	CMDSH_COMBINE_OUTPUT = 1 << 3,
	CMDSH_TTY_WINDOW     = 1 << 4,
	CMDSH_TTY_STDIN      = 1 << 5,
	CMDSH_TTY_STDOUT     = 1 << 6,
	CMDSH_TTY_STDERR     = 1 << 7,
};

struct cmdsh_flags_desc {
	char str[sizeof(CMDSH_FLAGS_DESC)];
};

struct cmdsh_request_header {
	uint32_t type;
	uint32_t seq;
	uint32_t cmdlen;
	uint32_t datalen;
	uint16_t ttyrow;
	uint16_t ttycol;
	char key[40];
};

struct cmdsh_reply_header {
	uint32_t type;
	uint32_t seq;
	uint32_t ack;
	uint32_t win;
	uint32_t errcode;
	uint32_t outlen;
	uint32_t errlen;
};

static __inline struct cmdsh_flags_desc cmdsh_flags(int flags)
{
	struct cmdsh_flags_desc desc = { CMDSH_FLAGS_DESC };
	int i;

	for (i = 0; i < sizeof(desc.str) - 1; i++) {
		if (!(flags & (1 << i)))
			desc.str[i] = '.';
	}

	return desc;
}

sds cmdsh_random_key();

#ifdef __cplusplus
}
#endif
