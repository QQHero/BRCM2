/*
 * Frontend command-line utility client for configuring Linux Embedded NVRAM
 *
 * Copyright 2022 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 * $Id: envramc.c 695725 2017-04-21 22:14:06Z $
 */

#include "envram.h"
#include <ctype.h>
#include <netdb.h>
#include <bcmnvram.h>

#if !defined(BCA_HNDROUTER)
#define ENVRAM_CMD_BUF_SIZE ENVRAM_MAX_SIZE4K
#else
#define ENVRAM_CMD_BUF_SIZE MAX_NVRAM_SPACE
#endif /* BCA_HNDROUTER */

typedef struct envramc_info {
	int conn_fd;
	char cmd_buf[ENVRAM_CMD_BUF_SIZE];
	char cur_cmd[10];
	unsigned int cmd_size;
} envramc_info_t;

envramc_info_t envramc_info;

/*
 * Print the usage of envramc utility
 */

void
usagec(void)
{
	fprintf(stderr,
		"envram client utility to read/modify the embedded nvram\n"
		"Options:						\n"
		"	get	-Get the value of the envram variable\n"
		"	set	-Set the value of the envram variable\n"
		"	unset	-Unset the envram variable\n"
		"	show	-Show all the variables in envram\n"
		"	commit	-Commit the changes in the envram variables\n"
		"	rall	-Replace the entire envram with the variables from given file\n"
		"	serv	-Specify the IP address and port number of the envram server\n"
		"								\n"
		"usage: envram <command> [serv ipaddr:port]\n"
		"	<command>:	[get name] | [set name=[value]] | [unset name] | [show]|\n"
		"			 [commit] | [rall file]\n"
		"NOTE:- Start the envrams on target to use this command\n");
		exit(0);
}

/*
 * Writes size bytes to a given device.
 * Handles the write failures because of signals.
 */
static int
swrite(int fd, char *buf, unsigned int size)
{
	int ret = 0, len = 0;

	do {
		errno = 0;
		ret = write(fd, &buf[len], size - len);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			else
				break;
		}
		if (ret > 0)
			len += ret;
	} while (len < size);

	return ((len > 0) ? len:ret);
}

/*
 * Reads size bytes into from a given device.
 * Handles the read failures because of signals.
 */
static int
sread(int fd, char *buf, unsigned int size)
{
	int ret = 0, len = 0;

	do {
		errno = 0;
		ret = read(fd, &buf[len], size - len);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			else
				break;
		}
		if (ret > 0)
			len += ret;
		} while ((len < size) && ret);

		return ((len > 0) ? len:ret);
}

/*
 * connects by default to 0.0.0.0:5152
 */
static int
envramc_connect_server(char **argv)
{
struct sockaddr_in servaddr;
unsigned int port = ENVRAMS_DFLT_PORT;
char *server_addr = "0.0.0.0", *addr;

	/* Get IP addr and port info from command line, if available */
	if (*++argv) {
		if (!strcmp(*argv, "serv")) {
			if (*++argv) {
				addr = strsep(argv, ":");
				if (addr) {
					server_addr = addr;
				}
				if (*argv && **argv) {
					port = strtol(*argv, NULL, 0);
				}
			} else usagec();
		} else usagec();
	}

	memset(&servaddr, 0, sizeof(servaddr));
	if ((envramc_info.conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		ENVRAMDBG("socekt failed: %s\n", strerror(errno));
		return -1;
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);

	if (isalpha(server_addr[0]) || (inet_pton(AF_INET,
		(const char *)server_addr, &servaddr.sin_addr) == 0)) {
		ENVRAMDBG("Invalid IPADDR: %s\n", server_addr);
		usagec();
	}

	if (connect(envramc_info.conn_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		ENVRAMDBG("connect failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

/*
 * Read and parse the envram variable info from the file
 * if the size required by the variables exceeds the 4K
 * max limit, then discard the command.
 */
static int
get_env_data(const char *file, char *data, int size)
{
	FILE *fp;
	int lineno = 0;
	char *c, *end = data;
	int len, total_size = size;

	if (!(fp = fopen(file, "rb"))) {
		ENVRAMDBG("failed opening %s: %s\n", file, strerror(errno));
		return -1;
	}

	while (fgets(end, size, fp)) {
		lineno++;
		/* chomp newline */
		if ((c = strchr(end, '\n')))
			*c = '\0';
		else {
			ENVRAMDBG("Envram variables can not exceed max: %d bytes\n", total_size);
			fclose(fp);
			return -1;
		}
		/* eat blank lines and comments */
		if (!*end || *end == '#')
			continue;
		/* parse name=value */
		if (!(c = strchr(end, '='))) {
			ENVRAMDBG("warning: syntax error in %s line %d\n", file, lineno);
			fclose(fp);
			return -1;
		}
		/* Length includes termiating null charecter */
		len = strlen(end) + 1;
		end += len;
		size -= len;
	}
	/* Server expects 2 null charecters to mark the end of data */
	*end = '\0';
	fclose(fp);
	return (end - data + 1);
}

/*
 * Based on the envramc arguments, prepare equivalent
 * command for the server.
 */
static int
envramc_make_cmd(char ***arg)
{
	char * buf = envramc_info.cmd_buf;
	char **argv = *arg;
	int  ret_val = 0;

	/* Skip program name */
	++argv;

	if (!*argv)
		usagec();

	/* Process the arguments */
	if (!strcmp(*argv, "get")) {
		if (*++argv) {
			/* size includes null character also */
			sprintf(envramc_info.cur_cmd, "get");
			envramc_info.cmd_size =
				sprintf(buf, "%s:%s", envramc_info.cur_cmd, *argv) + 1;
		} else usagec();
	} else if (!strcmp(*argv, "set")) {
		if (*++argv) {
			/* Make sure it is in name=value form */
			char *name = strchr(*argv, '=');
			sprintf(envramc_info.cur_cmd, "set");
			if (!name) usagec();
			envramc_info.cmd_size =
				sprintf(buf, "%s:%s", envramc_info.cur_cmd, *argv) + 1;
		} else usagec();
	} else if (!strcmp(*argv, "unset")) {
		if (*++argv) {
			sprintf(envramc_info.cur_cmd, "unset");
			envramc_info.cmd_size =
				sprintf(buf, "%s:%s", envramc_info.cur_cmd, *argv) + 1;
		}
	} else if (!strcmp(*argv, "show")) {
		sprintf(envramc_info.cur_cmd, "show");
		envramc_info.cmd_size = sprintf(buf, "%s:", envramc_info.cur_cmd) + 1;
	} else if (!strcmp(*argv, "commit")) {
		sprintf(envramc_info.cur_cmd, "commit");
		envramc_info.cmd_size =  sprintf(buf, "%s:", envramc_info.cur_cmd) + 1;
	} else if (!strcmp(*argv, "rall")) {
		if (*++argv) {
			char *file = *argv;
			char *data;
			int len, size;

			sprintf(envramc_info.cur_cmd, "rall");
			len  = sprintf(buf, "%s:", envramc_info.cur_cmd);
			data = &buf[len];
			size = sizeof(envramc_info.cmd_buf) - len;
			if ((ret_val = get_env_data(file, data, size)) < 0)
				return -1;
			envramc_info.cmd_size = ret_val;
			envramc_info.cmd_size += len;

		} else
			usagec();
	} else usagec();

	*arg = argv;

	return 0;
}

int
main(int argc, char **argv)
{
	int rcount = 0;

	/* Create the envram command for server */
	if (envramc_make_cmd(&argv) <  0)
		return 1;

	/* Connect to the envram server */
	if (envramc_connect_server(argv) < 0)
		return 2;

	/* Send the command to server */
	if (swrite(envramc_info.conn_fd, envramc_info.cmd_buf, envramc_info.cmd_size)
		 < envramc_info.cmd_size) {
		ENVRAMDBG("Command send failed");
		return 3;
	}

	/* Help server get the data till EOF */
	shutdown(envramc_info.conn_fd, SHUT_WR);

	/* Process the response */
	if ((rcount = sread(envramc_info.conn_fd, envramc_info.cmd_buf,
		(ENVRAM_CMD_BUF_SIZE - 1))) < 0) {
		ENVRAMDBG("Response receive failed: %s\n", strerror(errno));
		return 4;
	}

	if (!strcmp(envramc_info.cur_cmd, "show")) {
		int len;
		char *end = envramc_info.cmd_buf;

		/*
		* Replace all the null charecters with next line
		*/
		for (; *end; end += (len + 1)) {
			len = strlen(end);
			end[len] = '\n';
		}
	}

	envramc_info.cmd_buf[rcount] = '\0';

	/* Display the response */
	if (envramc_info.cmd_buf[0]) printf("%s\n", envramc_info.cmd_buf);

	return 0;
}
