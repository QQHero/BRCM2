#pragma once

#define EV_STANDALONE		1
//#define EV_FEATURES		0
#define EV_USE_EPOLL		1
#define EV_USE_POLL			0
#define EV_MULTIPLICITY		0
#define EV_MINPRI			0
#define EV_MAXPRI			0
#define EV_NO_THREADS		1
#define EV_ASYNC_ENABLE		0
#define EV_CHILD_ENABLE		1
#define EV_FORK_ENABLE		0
#define EV_IDLE_ENABLE		0
#define EV_CHECK_ENABLE		0
#define EV_PREPARE_ENABLE	0
#define EV_STAT_ENABLE		0
#define EV_PERIODIC_ENABLE	0
#define EV_SIGNAL_ENABLE	1
#define EV_EMBED_ENABLE		0
#define EV_CLEANUP_ENABLE	0

#include "ev.h"
