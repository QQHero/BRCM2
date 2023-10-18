#pragma once

enum {
	CM_PROCESS_TTY_STDIN  = 1 << STDIN_FILENO,
	CM_PROCESS_TTY_STDOUT = 1 << STDOUT_FILENO,
	CM_PROCESS_TTY_STDERR = 1 << STDERR_FILENO,
	CM_PROCESS_COMBINE_OUTPUT = 1 << 5,
	//CM_PROCESS_INHERIT_STDIN  = 1 << 6,
	//CM_PROCESS_INHERIT_STDOUT = 1 << 7,
	//CM_PROCESS_INHERIT_STDERR = 1 << 8,
	CM_PROCESS_STDIN_EOF  = 1 << 10,
};

struct cm_process;
typedef void (*cm_process_start_cb)(struct cm_process *cp);
typedef void (*cm_process_stop_cb)(struct cm_process *cp, int status);
typedef void (*cm_process_output_cb)(struct cm_process *cp, sds data);

struct cm_process {
	ev_child ev;
	struct cm_io stdio[3];
	int pty[2];
	// options
	uint32_t flags;
	cm_process_start_cb on_start;
	cm_process_stop_cb on_stop;
	cm_process_output_cb on_stdout;
	cm_process_output_cb on_stderr;
	sds stdin_buffer;
};

static __inline int cm_process_pid(struct cm_process *cp)
{
	return cp->ev.pid;
}

void cm_process_init(struct cm_process *cp);
int cm_process_spawn(struct cm_process *cp, const char *file, ...);
int cm_process_kill(struct cm_process *cp, int signum);
void cm_process_feed(struct cm_process *cp, sds data);
int cm_process_set_tty_window(struct cm_process *cp, unsigned short row, unsigned short col);
