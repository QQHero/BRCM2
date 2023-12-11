/*
 * Frontend command-line utility server for configuring Linux Embedded NVRAM
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
 * $Id: envrams.c 695725 2017-04-21 22:14:06Z $
 */

#include "envram.h"
#include <signal.h>
#ifdef LINUX26
#include <mtd/mtd-user.h>
#else
#include <linux/mtd/mtd.h>
#endif
#include <bcmnvram.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <sbsdram.h>
#include <LzmaLib.h>

#define  CFE_BOOT_PART        "boot"		/* Flash boot partition is identified
						 * as "boot" in /proc/mtd
						 */

typedef struct envram_info {
	int fd;			  /* descriptor for the flash device holding envram  */
	int offset;		  /* Envram offset within flash			     */
	int size_left;		  /* Size of envram space available                  */
	int envram_max_size;      /* Max size of envram space available              */
	struct nvram_header *nvh; /* points to the nvram header in envram	     */
	mtd_info_t mtd_info;      /* Flash boot partition information		     */
	erase_info_t esector;     /* Sector containing envram			     */
	char *env_ptr;		  /* points to envram information below nvram header */
	char *env_end_ptr;	  /* points to end of envram information	     */
	char *sect_ptr;		  /* points to sector containing envram information  */
	char *envram_dev_name;	  /* Name of the device file pointing to flash boot
				   * partition
				   */
	char *nvram;              /* Space to uncompress and work on the envram  */
} envram_info_t;

typedef struct cons_dev {
	char *devname;
	int  cns_fd;
} cons_dev_t;

typedef struct envrams_info {
	envram_info_t envinfo;		/* envram info at server side				*/
	int  listen_fd;			/* server listens on this fd				*/
	int  conn_fd;			/* Clients connection fd				*/
	cons_dev_t cons;
} envrams_info_t;

envrams_info_t envrams_info = {
	envinfo: {
		fd: -1,
		envram_max_size: ENVRAM_MAX_SIZE4K,
		nvh: NULL,
		envram_dev_name: NULL
	},
	listen_fd: -1,
	conn_fd: -1
};

static int envram_set(char *, char *);

uint envram_maxsize_override = 0;

/*
 * Print the usage of envramc utility
 */

void
usage(void)
{
	printf(" \nEnvram server for accesing the envram region\n"
		"Options:				       \n"
		"	port	-Port number for the server to listen\n"
		"	-f	-Run this server in forground\n"
		"       -l      -Envram maximum size in KB, if not sure of exact\n"
		"                envram max size, you may corrupt CFE, recomended\n"
		"		 max size 1 or 4\n"
		"				\n"
		"usage: envrams [port portnum] [-f] [-l size]\n");
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

#if !defined(BCA_HNDROUTER)

#define ENVRAM_PATH_MAX   256

/*
 * Open an MTD device
 * @param       mtd     path to or partition name of MTD device
 * @return      return value of open()
 */
int
mtd_open(const char *mtd)
{
	FILE *fp;
	char dev[ENVRAM_PATH_MAX];
	envram_info_t *envs = &envrams_info.envinfo;
	int i;

	if ((fp = fopen("/proc/mtd", "r"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd)) {
#ifndef LINUX26
				snprintf(dev, sizeof(dev), "/dev/mtd/%d", i);
#else
				snprintf(dev, sizeof(dev), "/dev/mtd%d", i);
#endif
				envs->envram_dev_name = (char *) malloc(strlen(dev)+1);
				if (!envs->envram_dev_name) {
					ENVRAMDBG("Malloc failed: %s\n", strerror(errno));
					fclose(fp);
					return -1;
				}
				strcpy(envrams_info.envinfo.envram_dev_name, dev);
				break;
			}
		}
		fclose(fp);
	}

	if (!envs->envram_dev_name) {
		ENVRAMDBG("Could not find flash boot device\n");
		usage();
	}
	return open(envs->envram_dev_name, O_RDWR);
}
#endif /* BCA_HNDROUTER */

#if !defined(BCA_HNDROUTER)
/*
 * Find the maximum allowable space in the envram region
 */
int
get_envram_max_size(void)
{
	struct nvram_header **envh = &envrams_info.envinfo.nvh;
	envram_info_t *env_info = &envrams_info.envinfo;
	char (* scan_begin)[ENVRAM_MAX_SIZE1K] =
		(char (*)[])ROUNDUP((unsigned int)env_info->env_end_ptr, ENVRAM_MAX_SIZE1K);
	unsigned int scan_end = (unsigned int)env_info->sect_ptr + ENVRAM_MAX_OFFSET;
	char *buf = (char *) (*envh);
	int *is_1ksize, *is_4ksize;
	unsigned int bytes_used = (unsigned int)(env_info->env_end_ptr + 1) - (unsigned int)(*envh);

	/* if user provides the maximum envram size then obey it */
	if (envram_maxsize_override) {
		if (envram_maxsize_override > bytes_used)
			return (envram_maxsize_override - 4);
		printf("Specified envram max size(0x%x) < used envram size(0x%x)\n",
			envram_maxsize_override, bytes_used);
		printf("Use a value > 0x%x for -l option\n", bytes_used);
		return 0;
	}

	/*
	 * Currently we have either 1K or 4K envram regions,
	 * to check the maximum size of the envram, we check for
	 * the marker ~NVRAM_MAGIC at 1K and 4K offset in the
	 * envram
	 */
	is_1ksize = (int *)&buf[ENVRAM_MAX_SIZE1K - sizeof(int)];
	is_4ksize = (int *)&buf[ENVRAM_MAX_SIZE4K - sizeof(int)];

	*is_1ksize = ltoh32((*is_1ksize));
	*is_4ksize = ltoh32((*is_4ksize));

	/*
	 * As of now following are the possible offset and size of envram in
	 * CFE image:
	 * 	Offset		Size		Image
	 *	----------------------------------------------
	 *	1K		1K		cfe compressed
	 *	1K		4K		cfe compressed
	 *	4K		4K		cfe uncompressed
	 *
	 * In the cfe image from build, envram region is filled with ~NVRAM_MAGIC
	 * pattern. nvserial identifies envram region in cfe image usin ~NVRAM_MAGIC
	 * pattern , and fills it with the given envram variables from board
	 * specific nvram txt file. Nvserial utility fills remaining bytes in envram
	 *  region with '\0' charecters, and leaves ~NVRAM_MAGIC number as last
	 * 4 bytes of envram region.
	 *
	 * To identify the envram region max length, we check for the ~NVRAM_MAGIC
	 * number at 1K and 4K offset from the begining of nvram header.
	 *
	 * We retain last 4 bytes at the end to store the marker, so we return
	 * we dont use sizeof(int) instead of 4, because nvserial uses digit 4.
	 */
	if (*is_4ksize == ~NVRAM_MAGIC) return (ENVRAM_MAX_SIZE4K - 4);
	if (*is_1ksize == ~NVRAM_MAGIC) return (ENVRAM_MAX_SIZE1K - 4);

	for (; (unsigned int)scan_begin < scan_end; scan_begin++) {
		unsigned char *buf1k = (unsigned char *)scan_begin;
		int i;

		/* check for non-ascii to find end of envram region */
		for (i = 0; i < ENVRAM_MAX_SIZE1K; i++) {
			if ((buf1k[i] != 0xff) && !isascii(buf1k[i])) break;
		}

		if (i < ENVRAM_MAX_SIZE1K)
			return ((TRUNC((unsigned int)&buf1k[i], ENVRAM_MAX_SIZE1K))
				 - ((unsigned int)(*envh) + 4));
	}

	/* right now we dont have envram size more than ENVRAM_MAX_SIZE4K */
	if ((unsigned int)scan_begin == scan_end) {return (ENVRAM_MAX_SIZE4K - 4);}

	return 0;
}
#endif /* BCA_HNDROUTER */

#if !defined(BCA_HNDROUTER)
/*
 *  Finds the ENVRAM section in flash. The beginning of ENVRAM configuration
 *  is expecte to be alligned to 0x400.
 *  o Get the size and erazesize of the flash partition.
 *  o Allocate erazesize buffer to hold the flash content.
 *  o Read erazesize of flash contents into buffer
 *    o Check for the ENVRAM at every 0x400 offset within the erazesize buffer
 *      starting at offset 0.
 *    o if valid ENVRAM header found then save the nvram header contents
 *      and the config variables in envinfo for processing further and return.
 *
 *     NOTE  o from shared/boot.S, it is obvious that the envram is 0x400 bytes.
 *          o If we change the size or offset allignment of the envram in
 *            shared/boot.S then dont expect this code to work.
 *          o End of the envram in the flash boot parition is marked by
 *            2 null charecters.
 */
static int
find_envram()
{
	int f_offset, rcount, readsize;
	envram_info_t *env_info = &envrams_info.envinfo;
	int *fd = &env_info->fd;
	mtd_info_t *mtdi = &env_info->mtd_info;
	erase_info_t *er_info = &env_info->esector;
	struct nvram_header **envh = &env_info->nvh, *nvh_at1k, *nvh_at4k;
	char *buf;

	if ((*fd = mtd_open(CFE_BOOT_PART)) < 0) {
		ENVRAMDBG("Could not open %s,%s",
			env_info->envram_dev_name, strerror(errno));
		return 0;
	}

	/* Make sure we have only once instance of envrams running */
	if (lockf(*fd, F_TLOCK, 0) < 0) {
		ENVRAMDBG("envrams already running\n");
		exit(0);
	}

	/* Get flash boot partition information */
	if (ioctl(*fd, MEMGETINFO, mtdi) != 0) {
		ENVRAMDBG("MEMGETINFO failed: %s\n", strerror(errno));
		return 0;
	}

	/* We have envram in starting at 0x400 offet in compressed CFE.
	 * or starting at 0x1000 offset in un-compressed CFE image.
	 * The envram may be of 1K or 4K size from above offsets.
	 * We are sure to have entire envram within 8K of flash boot partition
	 * We can update a minimum of one sector in the flash, so we
	 * if a sector is greater than 8K then, it is worth reading 1 sector.
	 * Otherwise read first 8K, from which we can surely makeout the
	 * location and size of envram.
	 */
	readsize = ROUNDUP(ENVRAM_MAX_OFFSET, mtdi->erasesize);

	/* Get a buffer to fit either 8K or erasesize what ever is big */
	if (!(env_info->sect_ptr = malloc(readsize))) {
		ENVRAMDBG("malloc failed: %s\n", strerror(errno));
		return 0;
	}

	f_offset = rcount = 0;

	if ((rcount = read(*fd, env_info->sect_ptr, readsize)) != readsize) {
		ENVRAMDBG("Failed reading flash: %s\n", strerror(errno));
		return 0;
	}

	nvh_at1k = (struct nvram_header *)&env_info->sect_ptr[ENVRAM_COMPRS_OFFSET];
	nvh_at4k = (struct nvram_header *)&env_info->sect_ptr[ENVRAM_UNCOMPRS_OFFSET];

	if (nvh_at1k->magic == NVRAM_MAGIC) {
		*envh = nvh_at1k;
		env_info->offset = ENVRAM_COMPRS_OFFSET;
	} else if (nvh_at4k->magic == NVRAM_MAGIC) {
		*envh = nvh_at4k;
		env_info->offset = ENVRAM_UNCOMPRS_OFFSET;
	} else {
		printf("No ENVRAM found\n");
		return 0;
	}
	buf = (char *) (*envh);
	/* DOUBT: is it ok to get the pointer to env_ptr based on size of struct */
	env_info->env_ptr = (char *) &(*envh)[1];

	if ((env_info->env_ptr[0] == 0x5d) &&
	    (env_info->env_ptr[1] == 0x00) &&
	    (env_info->env_ptr[2] == 0x00)) {
		unsigned int destLen;
		unsigned int srcLen;

		/* We have a compressed envram
		 * decompress into another buffer we will recompress
		 * that buffer every time we write a bit of data,
		 * set the max size for NVRAM_SPACE since we can't predict the
		 * space available, but return an error if we overrun the space
		 * when trying to update a variable
		 */

		if (!(env_info->nvram = malloc(MAX_NVRAM_SPACE))) {
			ENVRAMDBG("malloc failed: %s\n", strerror(errno));
			return 0;
		}
		destLen = MAX_NVRAM_SPACE-NVRAM_HEADER_SIZE;
		srcLen =  get_envram_max_size() - LZMA_PROPS_SIZE;
		LzmaUncompress((unsigned char *)&env_info->nvram[NVRAM_HEADER_SIZE], &destLen,
		               (unsigned char *)&env_info->env_ptr[LZMA_PROPS_SIZE], &srcLen,
		               (unsigned char *)&env_info->env_ptr[0], LZMA_PROPS_SIZE);

		memcpy(env_info->nvram, buf, NVRAM_HEADER_SIZE);
		env_info->env_ptr = &env_info->nvram[NVRAM_HEADER_SIZE];

		for (buf = env_info->env_ptr; *buf; buf += strlen(buf)+1);
		env_info->env_end_ptr = buf;

		/* Get the maximum size of the envram space */
		env_info->envram_max_size = MAX_NVRAM_SPACE;

		/* get the start address of the sector containing envram */
		er_info->start = TRUNC(env_info->offset, mtdi->erasesize);
		er_info->length = readsize;
		env_info->size_left = env_info->envram_max_size -
		        ((int) env_info->env_end_ptr - (int)(env_info->nvram));

	} else {
		env_info->nvram = NULL;

		for (buf = env_info->env_ptr; *buf; buf += strlen(buf)+1);
		env_info->env_end_ptr = buf;

		/* Get the maximum size of the envram space */
		if (!(env_info->envram_max_size = get_envram_max_size())) {
			ENVRAMDBG("Envram offset: 0x%x\n", env_info->offset);
			ENVRAMDBG("Could not find the envram max length\n");
			return 0;
		}

		/* Plant end of envram in cases where did not exist */
		buf = (char *) (*envh);
		*((int *)&buf[env_info->envram_max_size]) = htol32(~NVRAM_MAGIC);
		/* get the start address of the sector containing envram */
		er_info->start = TRUNC(env_info->offset, mtdi->erasesize);
		er_info->length = readsize;
		env_info->size_left = env_info->envram_max_size -
		        ((int) env_info->env_end_ptr - (int)(*envh));
	}

	return 1;
}
#else /* BCA_HNDROUTER */

#define ENVRAM_NAME_PATH "/mnt/nvram/nvram.nvm"
/*
 *  Find the MFG NVRAM file on the flash. MFG NVRAM is a regular file on
 *  UBI volume attached on dedicated MTD partition (misc1 by default).
 *  o Mountingthe MFG NVRAM UBI FS as read-only.
 *  o Allocate memory buffer to hold the NVRAM file content.
 *  o Read NVRAM file contents into buffer.
 *  o Un-mounting the MFG NVRAM UBI FS.
 *
 * NOTE: Compressed envram is not supported.
 */
static int
find_envram()
{
	envram_info_t *env_info = &envrams_info.envinfo;
	FILE *fd = NULL;
	char *buf;
	int ret;
	size_t rcount = 0;
	bool nvram_mounted = FALSE;

	/* Mount manufacturing NVRAM UBI FS on dedicated MTD partition as read-only.*/
	ret = system("/etc/init.d/hndnvram.sh mount_ubi -r >/dev/null 2>/dev/null");
	if (ret != -1 && !WIFEXITED(ret)) {
		ENVRAMDBG("NVRAM FS mount failed.\n");
		ret = 0;
		goto fail;
	}
	nvram_mounted = TRUE;

	ret = 1;
	if ((fd = fopen(ENVRAM_NAME_PATH, "r")) == NULL) {
		ENVRAMDBG("Could not open %s,%s",
			ENVRAM_NAME_PATH, strerror(errno));
		ret = 0;
		goto fail;
	}

	/* Allocate a buffer slightly bigger then the original NVRAM
	 * size to have enough space for new vars=values tuples.
	 */
	env_info->env_ptr = NULL;
	if ((env_info->env_ptr = malloc(MAX_NVRAM_SPACE)) == NULL) {
		ENVRAMDBG("malloc failed: %s\n", strerror(errno));
		ret = 0;
		goto fail;
	}

	clearerr(fd);
	rcount = fread(env_info->env_ptr, 1, MAX_NVRAM_SPACE, fd);
	if (ferror(fd)) {
		ENVRAMDBG("Failed reading NVRAM file: %s (rcount=%d)\n", strerror(errno), rcount);
		ret = 0;
		goto fail;
	}

	if (feof(fd) && (rcount < MAX_NVRAM_SPACE)) {
		/* Assumed that envram is not compressed.
		 * Compressed envram is not supported yet.
		 */
		for (buf = env_info->env_ptr; *buf; buf += strlen(buf)+1);
		env_info->env_end_ptr = buf;

		env_info->envram_max_size = MAX_NVRAM_SPACE;
		env_info->size_left = env_info->envram_max_size -
			((int)env_info->env_end_ptr - (int)env_info->env_ptr);
	}

fail:
	if ((ret == 0) && (env_info->env_ptr != NULL))
		free(env_info->env_ptr);

	if (fd != NULL)
		fclose(fd);

	if (nvram_mounted)
		system("/etc/init.d/hndnvram.sh umount_ubi >/dev/null 2>/dev/null");

	return ret;
}

#endif /* BCA_HNDROUTER */

#if !defined(BCA_HNDROUTER)
/*
 * Copy the working copy envram back to the envh, compressing it in the process
 */
static int envram_compress(void)
{
	struct nvram_header **envh = &envrams_info.envinfo.nvh;
	envram_info_t *env_info = &envrams_info.envinfo;
	unsigned int propsize;
	unsigned int destlen;
	unsigned char *cp;
	int res;

	propsize = LZMA_PROPS_SIZE;
	destlen = get_envram_max_size();
	cp = ((unsigned char *)(*envh))+NVRAM_HEADER_SIZE;

	res = LzmaCompress(&cp[LZMA_PROPS_SIZE], &destlen,
	                   (unsigned char *)&env_info->nvram[NVRAM_HEADER_SIZE],
	                   env_info->env_end_ptr-env_info->nvram,
	                   &cp[0], &propsize,
	                   -1, 1<<16, -1, -1, -1, -1, -1);
	if (res != SZ_OK) {
		ENVRAMDBG("Compression Error\n");
		return 0;
	}
	return 1;

}
#endif /* BCA_HNDROUTER */

/*
 * If found, return the pointer to the value of the envram variable
 * else return NULL.
 */

char *
envram_get(char *envstr)
{
	int len, i;
	char *name = envrams_info.envinfo.env_ptr;

	len = strlen(envstr);

	for (i = 0; *name; name += strlen(name)+1, i++) {
		if (!strncmp(name, envstr, len) && (*(name + len) == '='))
			return (name+len+1);
	}
	return NULL;
}

#if !defined(BCA_HNDROUTER)
/*
 *   Re-Initializes the nvram header and writes the updated nvram variables
 *   and header into boot partition of the flash:
 *   o  Calculate and update the CRC for the header starting from len
 *      field of nvram.
 *   o  Seek the offset to the beggining of the ENVRAM sector in the
 *	flash boot partition.
 *   o  Eraze the ENVRAM sector in flash boot partition.
 *   o  Write the nvram header into flash.
 *   o  close the device to make sure all the data is writen to flash.
 */
int
envram_commit()
{
	char buf[11]; /* Space for 0xXXXXXXXX and '\0' */
	envram_info_t *env_info = &envrams_info.envinfo;
	mtd_info_t *mtdi = &env_info->mtd_info;
	erase_info_t *er_info = &env_info->esector;
	struct nvram_header *envh = env_info->nvh;
	uint32 *src, *dst, i, var;

	/* Set special SDRAM parameters */
	if (!envram_get("sdram_init")) {
		sprintf(buf, "0x%04X", (uint16)(envh->crc_ver_init >> 16));
		envram_set("sdram_init", buf);
	}
	if (!envram_get("sdram_config")) {
		sprintf(buf, "0x%04X", (uint16)(envh->config_refresh & 0xffff));
		envram_set("sdram_config", buf);
	}
	if (!envram_get("sdram_refresh")) {
		sprintf(buf, "0x%04X", (uint16)((envh->config_refresh >> 16) & 0xffff));
		envram_set("sdram_refresh", buf);
	}
	if (!envram_get("sdram_ncdl")) {
		sprintf(buf, "0x%08X", envh->config_ncdl);
		envram_set("sdram_ncdl", buf);
	}

	/* Store last 11 bytes of header as Little-endian */
	envh->crc_ver_init = htol32((envh->crc_ver_init & NVRAM_CRC_VER_MASK));
	envh->config_refresh = htol32(envh->config_refresh);
	envh->config_ncdl = htol32(envh->config_ncdl);

	src = (uint32 *) &envh[1];
	dst = src;

	for (i = sizeof(struct nvram_header); i < envh->len &&
		i < env_info->envram_max_size; i += 4)
		*dst++ = htol32(*src++);

	/* Calculate the new length and update in the header */
	envh->len = (uint32)ROUNDUP((((int)env_info->env_end_ptr - (int)envh) + 1), 4);

	/* calculate the new header CRC and update in the nvram header */
	envh->crc_ver_init |= hndcrc8((unsigned char *) envh + NVRAM_CRC_START_POSITION,
		envh->len - NVRAM_CRC_START_POSITION, CRC8_INIT_VALUE) & 0xff;

	/* If we had a compressed nvram, compress it and copy it into the sector copy */
	if (env_info->nvram) {
		if (!envram_compress()) {
			/* We have a compression error */
			ENVRAMDBG("Compression failed");
			return -1;
		}
	}

	var = strtoul(envram_get("sdram_config"), NULL, 16) & 0xffff;
	if ((ltoh32(envh->config_refresh) & 0xffff) != var) {
		envh->config_refresh = htol32(ltoh32(envh->config_refresh & 0xffff0000) | var);
	}

	/*
	 * Open the flash device file if it is closed
	 * and seek only if device was  already open
	 */
	if ((env_info->fd >= 0) && (lseek(env_info->fd, er_info->start, SEEK_SET) < 0)) {
		ENVRAMDBG("seek set failed: %s", strerror(errno));
		return -1;
	} else if ((env_info->fd = open(env_info->envram_dev_name, O_RDWR)) < 0) {
		ENVRAMDBG("open %s failed: %s", env_info->envram_dev_name, strerror(errno));
		return -1;
	}

	/*
	 * UNLOCK the sector which contains the envram
	 * Dont require to check the return value, because
	 * some flash may not support unlock.
	 * We use unlock for comaptibility with all kinds
	 * of flash
	 */
	ioctl(env_info->fd, MEMUNLOCK, er_info);

	/* erase the sector which contains the envram */
	if (ioctl(env_info->fd, MEMERASE, er_info) < 0) {
		ENVRAMDBG("MEMERASE failed: %s", strerror(errno));
		return -1;
	}

	/* Writing in to the flash */
	if (write(env_info->fd, (char *)env_info->sect_ptr, mtdi->erasesize) != mtdi->erasesize) {
		ENVRAMDBG("write failed: %s", strerror(errno));
		return -1;
	}

	/* close to make sure the data is flushed to flash */
	close(env_info->fd);
	env_info->fd = -1;

	return 0;
}
#else /* BCA_HNDROUTER */
int
envram_commit()
{
	envram_info_t *env_info = &envrams_info.envinfo;
	int ret;
	bool nvram_mounted = FALSE;
	size_t len, wcount;
	FILE *fd = NULL;

	/* Mount manufacturing NVRAM UBI FS on dedicated MTD partition as read/write. */
	ret = system("/etc/init.d/hndnvram.sh mount_ubi >/dev/null 2>/dev/null");
	if (ret != -1 && !WIFEXITED(ret)) {
		ENVRAMDBG("NVRAM FS mount failed.\n");
		ret = 0;
		goto fail;
	}
	nvram_mounted = TRUE;

	ret = 1;
	if ((fd = fopen(ENVRAM_NAME_PATH, "w")) == NULL) {
		ENVRAMDBG("Could not open %s,%s",
			ENVRAM_NAME_PATH, strerror(errno));
		ret = 0;
		goto fail;
	}

	/* Calculate the new length */
	len = ((int)env_info->env_end_ptr - (int)env_info->env_ptr) + 1;
	clearerr(fd);
	wcount = fwrite(env_info->env_ptr, 1, len, fd);
	if (wcount != len || ferror(fd)) {
		ENVRAMDBG("Failed write NVRAM file. %s. (len:%d, wcoount:%d)\n",
				  strerror(errno), len, wcount);
		ret = 0;
		goto fail;
	}

fail:
	if (fd != NULL)
		fclose(fd);

	if (nvram_mounted)
		system("/etc/init.d/hndnvram.sh umount_ubi >/dev/null 2>/dev/null");

	return ret;
}
#endif /* BCA_HNDROUTER */

/*
 * Removes a specified envram variable
 */
unsigned int
envram_unset(char *envstr)
{
	int len, i;
	envram_info_t *env_info = &envrams_info.envinfo;
	char *name = env_info->env_ptr;
#if !defined(BCA_HNDROUTER)
	struct nvram_header *envh = env_info->nvh;
#endif /* BCA_HNDROUTER */

	len = strlen(envstr);

	for (i = 0; *name; name += strlen(name)+1, i++) {
		if (!strncmp(name, envstr, len) && (*(name + len) == '=')) {
			char *p = name;
			name += (strlen(name)+1);
			/* copy the tail, only if the removal string is not at end */
			if (!(*name))
				len = 0;
			else {
				len = env_info->env_end_ptr - name;
				memcpy(p, name, len);
			}
			p[len] = '\0';
			env_info->env_end_ptr = p + len;
			if (env_info->nvram)
				env_info->size_left = env_info->envram_max_size -
				        ((int)env_info->env_end_ptr - (int)env_info->nvram);
			else
#if !defined(BCA_HNDROUTER)
				env_info->size_left = env_info->envram_max_size -
					((int)env_info->env_end_ptr - (int)envh);
#else
			env_info->size_left = env_info->envram_max_size -
				((int)env_info->env_end_ptr - (int)env_info->env_ptr);
#endif /* BCA_HNDROUTER */
			return 1;
		}
	}

	return -1;
}

/*
 *  Add an envram variable to the end:
 *  o  If the new envram variable along with value is already
 *     present, then return.
 *  o  Find if the envram variable matches but not value.
 *     o If present then remove it.
 *  o  Check if there is enough space left to add new variable
 *  o  add the new variable with value and terminating null charecter
 *  o  Update the new size of the envram
 */
static int
envram_set(char *envstr, char *val)
{
	int  len;
	char *envvar = NULL;
	char *new_envr;
	envram_info_t *env_info = &envrams_info.envinfo;
	char **end = &env_info->env_end_ptr;
#if !defined(BCA_HNDROUTER)
	struct nvram_header *envh = env_info->nvh;
#endif /* BCA_HNDROUTER */

	/* Length of variable, value including '=' and '\0' */
	len = strlen(envstr) + strlen(val) + 2;

	/* Nothing to do if we already have matching variable and value */
	if ((envvar = envram_get(envstr))&& !strcmp(envvar, val))
		return 0;

	/* If we already have variable but value does not match then remove it */
	envram_unset(envstr);

	/* check for length including null charecter */
	if (len > env_info->size_left) {
		ENVRAMDBG("No space in envram, (%d left)\n", env_info->size_left);
		return -1;
	}

	if (!(new_envr = (char *)malloc(len))) {
		ENVRAMDBG("malloc failed:%s\n", strerror(errno));
		return -2;
	}

	sprintf(new_envr, "%s=%s", envstr, val);

	/* Add the new variable at the end */
	memcpy(*end, new_envr, len);
	free(new_envr);
	(*end)[len] = '\0';
	env_info->env_end_ptr = *end + len;

#if !defined(BCA_HNDROUTER)
	env_info->size_left = env_info->envram_max_size -
	        ((int)env_info->env_end_ptr - (int)envh);
#else
	env_info->size_left = env_info->envram_max_size -
		((int)env_info->env_end_ptr - (int)env_info->env_ptr);
#endif /* BCA_HNDROUTER */
	return 0;
}

#if !defined(BCA_HNDROUTER)
/* Run in background */
static void
daemonize()
{
	int i;

	i = fork();
	if (i<0) {  /* fork error */
		ENVRAMDBG("Fork failed: %s\n", strerror(errno));
		exit(1);
	}
	if (i > 0)
		exit(0); /* parent exits */
	/* child (daemon) continues */
	setsid(); /* obtain a new process group */
	for (i = getdtablesize(); i >= 0; --i)
		close(i);  /* close all descriptors */
	envrams_info.cons.devname = "/dev/null";
	if ((envrams_info.cons.cns_fd = open(envrams_info.cons.devname, O_RDWR)) < 0) {
		ENVRAMDBG("open %s failed: %s", envrams_info.cons.devname, strerror(errno));
		exit(1);
	}
	dup(envrams_info.cons.cns_fd);
	dup(envrams_info.cons.cns_fd); /* handle standart I/O */

	umask(027); /* set newly created file permissions */
	signal(SIGCHLD, SIG_IGN); /* ignore child */
	signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
}

#else /* BCA_HNDROUTER */
/* dummy sig handler */
static void signal_handler(int signumber)
{
	ENVRAMDBG("@pid:%d received signal %d\n", getpid(), signumber);
	return;
}

static void resume_parent()
{
	int parent_pid = getppid();
	ENVRAMDBG("child@pid=%d  sending SIGUSR1 to parent@pid:%d\n",getpid(),  parent_pid);
	kill(parent_pid, SIGUSR1);
}

/*
 * Create a child process.
 * Parent process will be suspended until "SIGUSR1" and "SIGCHLD"
 * or other unmaskabe job control signals arrives from child
 * process.
 * The child process will send the following signals:
 *   o  SIGUSR1 - if initilized (NVRAM UBI FS is mounted) and
 *                ready to precess a commands (communication
 *                socket is opened)
 *   o  SIGCHLD - if exits when error occurs.
 * In both cases the parent process will be resumed from suspended
 * state and exit.
 */
static void
daemonize()
{
	int i;
	sigset_t masked_sigs; /* the signals which are blocked */

	/* Get all of the defined signals */
	sigfillset(&masked_sigs);

	/* removes SIGUSR1 and SIGCHLD signals from the set */
	sigdelset(&masked_sigs, SIGUSR1);
	sigdelset(&masked_sigs, SIGCHLD);

	/* Define dummy handler for SIGUSR1 and SIGCHLD */
	signal(SIGUSR1, signal_handler);
	signal(SIGCHLD, signal_handler);

	i = fork();
	if (i<0) {  /* fork error */
		ENVRAMDBG("Fork failed: %s\n", strerror(errno));
		exit(1);
	}
	if (i > 0) {
		ENVRAMDBG("parent@pid:%d waiting for child@pid:%d\n", getpid(), i);
		/*
		 * Temporarily masking all the signals of the parent process
		 * and suspending the parent process until one of the signals
		 * "SIGUSR1" and "SIGCHLD" arrives.
		 * If the parent process is woken up by delivery of SIGUSR1
		 * and SIGCHLD signal that invokes a  handler function, and
		 * the handler function returns, then sigsuspend also returns.
		 * Child will signal parent process with "SIGUSR1" once the
		 * child process is initilized and ready to precess a commands.
		 * That is NVRAM UBI FS is mounted, and communication socket is
		 * opened. Child will signal parent with "SIGCHLD" if child
		 * process will exit in case if error occurs.
		 */
		sigsuspend(&masked_sigs);
		ENVRAMDBG("parent@pid:%d exiting\n", getpid());
		exit(0); /* parent exits */
	}
	/* child (daemon) continues */
	setsid(); /* obtain a new process group */

	for (i = getdtablesize(); i >= 0; --i)
		close(i);  /* close all descriptors */
	envrams_info.cons.devname = "/dev/null";
	if ((envrams_info.cons.cns_fd = open(envrams_info.cons.devname, O_RDWR))<0) {
		ENVRAMDBG("open %s failed: %s", envrams_info.cons.devname, strerror(errno));
		exit(1);
	}
	dup(envrams_info.cons.cns_fd);
	dup(envrams_info.cons.cns_fd); /* handle standart I/O */

	umask(027); /* set newly created file permissions */
	signal(SIGCHLD, SIG_IGN); /* ignore child */
	signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
}
#endif /* BCA_HNDROUTER */

/*
 * Initialize the TCP socket for clients to connect
 */
static int
envrams_sock_init(unsigned int port)
{
	struct sockaddr_in s_sock;

	envrams_info.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (envrams_info.listen_fd < 0) {
		ENVRAMDBG("Socket open failed: %s\n", strerror(errno));
		return -1;
	}

	memset(&s_sock, 0, sizeof(struct sockaddr_in));
	s_sock.sin_family = AF_INET;
	s_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	s_sock.sin_port = htons(port);

	if (bind(envrams_info.listen_fd, (struct sockaddr *)&s_sock,
		sizeof(struct sockaddr_in)) < 0) {
		ENVRAMDBG("Socket bind failed: %s\n", strerror(errno));
		return -1;
	}

	if (listen(envrams_info.listen_fd, 5) < 0) {
		ENVRAMDBG("Socket listen failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;

}

/*
 * o Validate the command
 * o Process the command from client
 * o respond back to client with success or failure message
 */
static void
envrams_proc_cmd()
{
	unsigned long  resp_size = 0;
	long rcount = 0;
	unsigned long  data_len = 0;
	static char *buf = NULL;
	char *c, *data, *resp_msg;
	envram_info_t *env = &envrams_info.envinfo;

	/*
	 * We allocate only once, and reuse it everytime
	 * we use 32K as the size of the buffer, since
	 * it maybe compressed later
	 */
	if (!buf && !(buf = (char *)malloc(MAX_NVRAM_SPACE))) {
		ENVRAMDBG("Failed malloc: %s\n", strerror(errno));
		return;
	}

	/* get command from client */
	if ((rcount = sread(envrams_info.conn_fd, buf, MAX_NVRAM_SPACE)) < 0) {
		ENVRAMDBG("Failed reading message from client: %s\n", strerror(errno));
		return;
	}

	/* Check if we have command and data in the expected order */
	if (!(c = strchr(buf, ':'))) {
		ENVRAMDBG("Missing Command\n");
		return;
	}
	*c++ = '\0';
	data = c;
	data_len = rcount - (c - buf);

	if (!strcmp(buf, "show")) {
		resp_msg = env->env_ptr;
		resp_size = env->env_end_ptr - env->env_ptr + 1;
	} else if (!strcmp(buf, "get")) {
		if ((resp_msg = envram_get(data))) {
			resp_size = strlen(resp_msg);
		} else buf[0] = '\0';
	} else if (!strcmp(buf, "set")) {
		int rc = 0;

		c = strsep(&data, "=");
		if ((rc = envram_set(c, data)) < 0) {
			if (rc == -1) resp_size = sprintf(buf, "No space, (%d left)",
				envrams_info.envinfo.size_left);
			else if (rc == -2) resp_size = sprintf(buf, "Malloc failed");
			resp_msg = buf;
		} else {
			buf[0] = '\0';
		}
	} else if (!strcmp(buf, "unset")) {
		envram_unset(data);
		buf[0] = '\0';
	} else if (!strcmp(buf, "rall")) {
#if !defined(BCA_HNDROUTER)
		int max_data_size = env->envram_max_size - sizeof(struct nvram_header);
#else
		int max_data_size = env->envram_max_size;
#endif /* BCA_HNDROUTER */

		if (data_len > max_data_size) {
			resp_size = sprintf(buf, "Max bytes allowed: %d", max_data_size);
			resp_msg = buf;
		} else {
			char *end = env->env_ptr;
			envram_info_t *env_info = &envrams_info.envinfo;
#if !defined(BCA_HNDROUTER)
			struct nvram_header *envh = env_info->nvh;
#endif /* BCA_HNDROUTER */

			memcpy(end, data, data_len);
			env_info->env_end_ptr = &end[data_len];
#if !defined(BCA_HNDROUTER)
			env_info->size_left = env_info->envram_max_size -
				((int)env_info->env_end_ptr-(int)envh);
#else
			env_info->size_left = env_info->envram_max_size -
				((int)env_info->env_end_ptr - (int)env_info->env_ptr);
#endif /* BCA_HNDROUTER */
			buf[0] = '\0';
		}
	} else if (!strcmp(buf, "commit")) {
		if (envram_commit() < 0) {
			resp_size = sprintf(buf, "Error Compressing ENVRAM");
			resp_msg = buf;
		}
		else
			buf[0] = '\0';
	} else {
		resp_size = sprintf(buf, "Invalid envram command:%s", buf);
		resp_msg = buf;
	}

	if (buf[0] == '\0') {
		resp_msg = buf;
		resp_size = 1;
	}

	if (swrite(envrams_info.conn_fd, resp_msg, resp_size) < 0)
		ENVRAMDBG("Failed sending message: %s\n", strerror(errno));
}

/*
 * Receivs and processes the commands from client
 * o Wait for connection from client
 * o Process the command and repond back to client
 * o close connection with client
 */
static int
envrams_proc_client_req()
{
	int    fd = -1;
	socklen_t clilen = sizeof(struct sockaddr);
	struct sockaddr_in cliaddr;

	while (1) {
		fd = accept(envrams_info.listen_fd, (struct sockaddr*)&cliaddr,
			&clilen);
		if (fd < 0) {
			if (errno == EINTR) continue;
			else {
				ENVRAMDBG("accept failed: %s", strerror(errno));
				return -1;
			}
		}

		envrams_info.conn_fd = fd;
		envrams_proc_cmd();
		close(envrams_info.conn_fd);
		envrams_info.conn_fd = -1;
	}
}

int
main(int argc, char **argv)
{
#if !defined(BCA_HNDROUTER)
	char *end;
#endif /* BCA_HNDROUTER */
	unsigned int background = 1;
	unsigned int  port = ENVRAMS_DFLT_PORT;
#if !defined(BCA_HNDROUTER)
	envram_info_t *env_info = &envrams_info.envinfo;
#endif /* BCA_HNDROUTER */

	if (argc > 6)
		usage();

	while (*++argv) {
		if (!strcmp(*argv, "port")) {
			if (*++argv) {
				port = strtol(*argv, NULL, 0);
			} else usage();
		} else if (!strcmp(*argv, "-f")) {
			background = 0;
		} else if (!strcmp(*argv, "-l")) {
			if (*++argv) {
				envram_maxsize_override = strtol(*argv, NULL, 0);
				envram_maxsize_override *= ENVRAM_MAX_SIZE1K;
			}
		} else usage();
	}

	/* Run in background by default */
	if (background) daemonize();

	/* Extract envram info from flash */
	if (!find_envram()) return -1;

	printf("Listening on port: %d\n", port);
#if !defined(BCA_HNDROUTER)
	printf("Envram offset: 0x%x\n"
		"Envram size: 0x%x\n"
		"Erase sector size: 0x%x\n"
		"Envram Magic: 0x%08x\n"
		"len: 0x%08x\n"
		"crc_ver_init: 0x%08x\n"
		"config_refresh: 0x%08x\n"
		"config_ncdl: 0x%08x\n",
		env_info->offset, env_info->envram_max_size,
		env_info->mtd_info.erasesize,
		env_info->nvh->magic, env_info->nvh->len,
		env_info->nvh->crc_ver_init, env_info->nvh->config_refresh,
		env_info->nvh->config_ncdl);
	for (end = env_info->env_ptr; *end; end += strlen(end) + 1)
		printf("%s\n", end);
#endif /* BCA_HNDROUTER */

	/* Initialize the connection for clients to connect */
	if (envrams_sock_init(port) < 0) return -1;

#if defined(BCA_HNDROUTER)
	/* The paren process is waiting for child process.*/
	resume_parent();
#endif /* BCA_HNDROUTER */

	/* Process commands from client and respond back */
	envrams_proc_client_req();

	return 0;
}
