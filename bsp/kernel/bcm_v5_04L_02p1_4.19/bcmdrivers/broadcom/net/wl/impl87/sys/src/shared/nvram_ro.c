/*
 * Read-only support for NVRAM on flash and otp.
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
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: nvram_ro.c 766665 2018-08-09 09:00:19Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <bcmnvram.h>
#include <sbchipc.h>
#include <bcmsrom.h>
#include <bcmotp.h>
#include <bcmdevs.h>
#include <sflash.h>
#include <hndsoc.h>

#ifdef BCMDBG_ERR
#define NVR_MSG(x) printf x
#else
#define NVR_MSG(x)
#endif	/* BCMDBG_ERR */

#define NUM_VSIZES 16
typedef struct _vars {
	struct _vars *next;
	int bufsz;		/* allocated size */
	int size;		/* actual vars size */
	char *vars;
	uint16 vsizes[NUM_VSIZES];
} vars_t;

#ifdef DONGLEBUILD
extern char *_vars_otp;
#endif

#define	VARS_T_OH	sizeof(vars_t)

static vars_t *vars = NULL;

#if !defined(DONGLEBUILD) && !defined(BCMDONGLEHOST) && !defined(BCM_BOOTLOADER)
#define NVRAM_FILE	1
#endif

#ifdef NVRAM_FILE
static int nvram_file_init(void* sih);
static int initvars_file(si_t *sih, osl_t *osh, char **nvramp, int *nvraml);
#endif

static char *findvar(char *vars_arg, char *lim, const char *name);

extern void nvram_get_global_vars(char **varlst, uint *varsz);
static char *nvram_get_internal(const char *name);
static int nvram_getall_internal(char *buf, int count);

static void
#if defined(WLTEST)
sortvars(si_t *sih, vars_t *new)
#else
	BCMATTACHFN(sortvars)(si_t *sih, vars_t *new)
#endif
{
	osl_t *osh = si_osh(sih);
	uchar *s = (uchar *) new->vars;
	int i;
	uchar *temp;
	uchar *c, *cend;
	uint8 *lp;

	/*
	 * Sort the variables by length.  Everything len NUM_VISZES or
	 * greater is dumped into the end of the area
	 * in no particular order.  The search algorithm can then
	 * restrict the search to just those variables that are the
	 * proper length.
	 */

	/* get a temp array to hold the sorted vars */
	/* freed in same function */
	temp = MALLOC_NOPERSIST(osh, NVRAM_ARRAY_MAXSIZE + 1);
	if (!temp) {
		NVR_MSG(("Out of memory for malloc for NVRAM sort"));
		return;
	}

	c = temp;
	lp = c++;

	/* Mark the var len and exp len as we move to the temp array */
	while ((s < ((uchar*)new->vars + new->size)) && *s) {
		uint8 len = 0;
		uchar *start = c;
		while ((*s) && (*s != '=')) {  /* Scan the variable */
			*c++ = *s++;
			len++;
		}

		/* ROMS have variables w/o values which we skip */
		if (*s != '=') {
			c = start;

		} else {
			*lp = len; 		/* Set the len of this var */
			lp = (uint8 *) c++;
			s++;
			len = 0;
			while (*s) { 		/* Scan the expr */
				*c++ = *s++;
				len++;
			}
			*lp = len; 		/* Set the len of the expr */
			lp = (uint8 *) c++;
			s++;
		}
	}

	cend = lp;

	s = (uchar *)new->vars;
	for (i = 1; i <= NUM_VSIZES; i++) {
		new->vsizes[i - 1] = (uint16) (s - (uchar*) new->vars);
		/* Scan for all variables of size i */
		for (c = temp; c < cend;) {
			uint len = *c++;
			if ((len == i) || ((i == NUM_VSIZES) && (len >= NUM_VSIZES))) {
				/* Move the variable back */
				while (len--) {
					*s++ = *c++;
				}

				/* Get the length of the expression */
				len = *c++;
				*s++ = '=';

				/* Move the expression back */
				while (len--) {
					*s++ = *c++;
				}
				*s++ = 0;	/* Reinstate string terminator */

			} else {
				/* Wrong size - skip to next in temp copy */
				c += len;
				c += *c + 1;
			}
		}
	}

	MFREE(osh, temp, NVRAM_ARRAY_MAXSIZE + 1);
}

#if defined(FLASH)
/** copy flash to ram */
static void
BCMATTACHFN(get_flash_nvram)(si_t *sih, struct nvram_header *nvh)
{
	osl_t *osh;
	uint nvs, bufsz;
	vars_t *new;

	osh = si_osh(sih);

	nvs = R_REG(osh, &nvh->len) - sizeof(struct nvram_header);
	bufsz = nvs + VARS_T_OH;

	/* nvram data freed in si_detach */
	if ((new = (vars_t *)MALLOC_NOPERSIST(osh, bufsz)) == NULL) {
		NVR_MSG(("Out of memory for flash vars\n"));
		return;
	}
	new->vars = (char *)new + VARS_T_OH;

	new->bufsz = bufsz;
	new->size = nvs;
	new->next = vars;
	vars = new;
	sortvars(sih, new);

#ifdef BCMJTAG
	if (BUSTYPE(sih->bustype) == JTAG_BUS) {
		uint32 *s, *d;
		uint sz = nvs;

		s = (uint32 *)(&nvh[1]);
		d = (uint32 *)new->vars;

		ASSERT(ISALIGNED((uintptr)s, sizeof(uint32)));
		ASSERT(ISALIGNED((uintptr)d, sizeof(uint32)));

		while (sz >= sizeof(uint32)) {
			*d++ = ltoh32(R_REG(osh, s++));
			sz -= sizeof(uint32);
		}
		if (sz) {
			union {
				uint32	w;
				char	b[sizeof(uint32)];
			} data;
			uint i;
			char *dst = (char *)d;

			data.w =  ltoh32(R_REG(osh, s));
			for (i = 0; i < sz; i++)
				*dst++ = data.b[i];
		}
	} else
#endif	/* BCMJTAG */
		bcopy((char *)(&nvh[1]), new->vars, nvs);

	NVR_MSG(("%s: flash nvram @ %p, copied %d bytes to %p\n", __FUNCTION__,
	         nvh, nvs, new->vars));
}
#endif /* FLASH */

#if defined(BCMUSBDEV) || defined(BCMSDIODEV) || defined(BCMHOSTVARS)
#if defined(BCMSDIODEV) || defined(BCMHOSTVARS)
extern char *_vars;
extern uint _varsz;
#endif
#ifndef CONFIG_XIP
extern uint8 embedded_nvram[];
#endif
#endif	/* BCMUSBDEV || BCMSDIODEV || BCMHOSTVARS */

int
BCMATTACHFN(nvram_init)(void *si)
{
#if defined(FLASH)
	uint idx;
	chipcregs_t *cc;
	si_t *sih;
	osl_t *osh;
	void *oh;
	struct nvram_header *nvh = NULL;
	uintptr flbase;
	struct sflash *info;
	uint32 cap = 0, off, flsz;
#endif /* FLASH */

	/* Make sure we read nvram in flash just once before freeing the memory */
	if (vars != NULL) {
		NVR_MSG(("nvram_init: called again without calling nvram_exit()\n"));
		return 0;
	}

#if defined(FLASH)
	sih = (si_t *)si;
	osh = si_osh(sih);

	/* Check for flash */
	idx = si_coreidx(sih);
	cc = si_setcoreidx(sih, SI_CC_IDX);
	ASSERT(cc);

	flbase = (uintptr)OSL_UNCACHED((void *)SI_FLASH2);
	flsz = 0;
	cap = R_REG(osh, &cc->capabilities);

	switch (cap & CC_CAP_FLASH_MASK) {
	case PFLASH:
		flsz = SI_FLASH2_SZ;
		break;

	case SFLASH_ST:
	case SFLASH_AT:
		if ((info = sflash_init(sih, cc)) == NULL)
			break;
		flsz = info->size;
		break;

	case FLASH_NONE:
	default:
		break;
	}

	/* If we found flash, see if there is nvram there */
	if (flsz != 0) {
		off = FLASH_MIN;
		nvh = NULL;
		while (off <= flsz) {
			nvh = (struct nvram_header *)(flbase + off - NVRAM_SPACE);
			if (R_REG(osh, &nvh->magic) == NVRAM_MAGIC)
				break;
			off <<= 1;
			nvh = NULL;
		};

		if (nvh != NULL)
			get_flash_nvram(sih, nvh);
	}

	/* Check for otp */
	if ((oh = otp_init(sih)) != NULL) {
		uint sz = otp_size(oh);
		uint bufsz = sz + VARS_T_OH;
		/* can be freed in same function */
		vars_t *new = (vars_t *)MALLOC_NOPERSIST(osh, bufsz);
		if (new != NULL)
			new->vars = (char *)new + VARS_T_OH;
		if (new == NULL) {
			NVR_MSG(("Out of memory for otp\n"));
		} else if (otp_nvread(oh, new->vars, &sz)) {
			NVR_MSG(("otp_nvread error\n"));
			MFREE(osh, new, bufsz);
		} else {
			new->bufsz = bufsz;
			new->size = sz;
			new->next = vars;
			vars = new;
			sortvars(sih, new);
		}
	}

	/* Last, if we do have flash but no regular nvram was found in it,
	 * try for embedded nvram.
	 * Note that since we are doing this last, embedded nvram will override
	 * otp, a change from the normal precedence in the designs that use
	 * the full read/write nvram support.
	 */
	if ((flsz != 0) && (nvh == NULL)) {
		nvh = (struct nvram_header *)(flbase + 1024);
		if (R_REG(osh, &nvh->magic) == NVRAM_MAGIC)
			get_flash_nvram(sih, nvh);
		else {
			nvh = (struct nvram_header *)(flbase + 4096);
			if (R_REG(osh, &nvh->magic) == NVRAM_MAGIC)
				get_flash_nvram(sih, nvh);
		}
	}

	/* All done */
	si_setcoreidx(sih, idx);
#endif /* FLASH */

#if defined(BCMUSBDEV) || defined(BCMSDIODEV) || defined(BCMHOSTVARS)
#if defined(BCMSDIODEV) || defined(BCMHOSTVARS)
	/* Honor host supplied variables and make them global */
	if (_vars != NULL && _varsz != 0)
		nvram_append(si, _vars, _varsz);
#endif	/* BCMSDIODEV || BCMHOSTVARS */
#ifndef CONFIG_XIP
#endif	/* CONFIG_XIP */
#endif	/* BCMUSBDEV || BCMSDIODEV || BCMHOSTVARS */

#ifdef NVRAM_FILE
	if ((BUSTYPE(((si_t *)si)->bustype) == PCI_BUS) ||
		(BUSTYPE(((si_t *)si)->bustype) == SI_BUS)) {

		if (nvram_file_init(si) != 0)
			return BCME_ERROR;
	}
#endif /* NVRAM_FILE */

	return 0;
}

#ifdef NVRAM_FILE
static int
nvram_file_init(void* sih)
{
	char *base = NULL, *nvp = NULL, *flvars = NULL;
	int err = 0, nvlen = 0;

	/* freed in same function */
	base = nvp = MALLOC_NOPERSIST(si_osh((si_t *)sih), MAXSZ_NVRAM_VARS);
	if (base == NULL)
		return BCME_NOMEM;

	/* Init nvram from nvram file if they exist */
	err = initvars_file(sih, si_osh((si_t *)sih), &nvp, (int*)&nvlen);
	if (err != 0) {
		NVR_MSG(("No valid NVRAM file present!!!\n"));
		goto exit;
	}
	if (nvlen) {
		/* nvram data freed in si_detach */
		flvars = MALLOC_NOPERSIST(si_osh((si_t *)sih), nvlen);
		if (flvars == NULL)
			goto exit;
	}
	else
		goto exit;

	bcopy(base, flvars, nvlen);
	err = nvram_append(sih, flvars, nvlen);

exit:
	MFREE(si_osh((si_t *)sih), base, MAXSZ_NVRAM_VARS);

	return err;
}

/** NVRAM file read for pcie NIC's */
static int
initvars_file(si_t *sih, osl_t *osh, char **nvramp, int *nvraml)
{
#if defined(BCMDRIVER)
	/* Init nvram from nvram file if they exist */
	char *nvram_buf = *nvramp;
	void	*nvram_fp = NULL;
	int ret = 0, len = 0;
	char *nvram_fname = sih->nvram_fname;

	if (nvram_fname == NULL) {
		nvram_fname = "nvram.txt"; /* default file name */
	}

	nvram_fp = (void*)osl_os_open_image(nvram_fname);
	if (nvram_fp != NULL) {
		if (!(len = osl_os_get_image_block(nvram_buf, MAXSZ_NVRAM_VARS, nvram_fp))) {
			NVR_MSG(("Could not read %s file\n", nvram_fname));
			ret = -1;
			goto exit;
		}
	}
	else {
		NVR_MSG(("Could not open %s file\n", nvram_fname));
		ret = -1;
		goto exit;
	}

	/* process nvram vars if user specified a text file instead of binary */
	len = process_nvram_vars(nvram_buf, len);

	nvram_buf += len;
	*nvram_buf++ = '\0';
	*nvramp = nvram_buf;
	*nvraml = len+1; /* add one for the null character */

exit:
	if (nvram_fp)
		osl_os_close_image(nvram_fp);

	return ret;
#else /* BCMDRIVER */
	return BCME_ERROR
#endif /* BCMDRIVER */
}

int
nvram_file_read(char **nvramp, int *nvraml)
{
	return initvars_file(NULL, NULL, nvramp, nvraml);
}
#endif /* NVRAM_FILE */

int
BCMATTACHFN(nvram_append)(void *si, char *varlst, uint varsz)
{
	uint bufsz = VARS_T_OH;
	vars_t *new;

	/* nvram data freed in si_detach */
	if ((new = MALLOC_NOPERSIST(si_osh((si_t *)si), bufsz)) == NULL)
		return BCME_NOMEM;

	new->vars = varlst;
	new->bufsz = bufsz;
	new->size = varsz;
	new->next = vars;
	sortvars((si_t *)si, new);

	vars = new;

	return BCME_OK;
}

void
nvram_get_global_vars(char **varlst, uint *varsz)
{
	*varlst = vars->vars;
	*varsz = vars->size;
}

void
BCMATTACHFN(nvram_exit)(void *si)
{
	vars_t *this, *next;
	si_t *sih;

	sih = (si_t *)si;
	this = vars;
#ifdef DONGLEBUILD
	if (this && this->vars && ((_vars_otp == this->vars) ||
#ifdef NVSRCX
		(srom_get_sromvars() == this->vars) ||
#endif
		FALSE)) {
		MFREE(si_osh(sih), this->vars, this->bufsz);
		_vars_otp = NULL;
#ifdef NVSRCX
		srom_set_sromvars(NULL);
#endif
	}
#else
	if (this)
		MFREE(si_osh(sih), this->vars, this->size);
#endif /* DONGLEBUILD */

	while (this) {
		next = this->next;
		MFREE(si_osh(sih), this, this->bufsz);
		this = next;
	}
	vars = NULL;
}

static char *
findvar(char *vars_arg, char *lim, const char *name)
{
	char *s;
	int len;

	len = strlen(name);

	for (s = vars_arg; (s < lim) && *s;) {
		if ((bcmp(s, name, len) == 0) && (s[len] == '='))
			return (&s[len+1]);

		while (*s++)
			;
	}

	return NULL;
}

#ifdef BCMSPACE
char *defvars = "il0macaddr=00:11:22:33:44:55\0"
		"boardtype=0xffff\0"
		"boardrev=0x10\0"
		"boardflags=8\0"
		"aa0=3\0"
		"sromrev=2";
#define	DEFVARSLEN	89	/* Length of *defvars */
#else /* !BCMSPACE */
char *defvars = "";
#endif	/* BCMSPACE */

static char *
nvram_get_internal(const char *name)
{
	vars_t *cur;
	char *v = NULL;
	const int len = strlen(name);

	for (cur = vars; cur; cur = cur->next) {
		/*
		 * The variables are sorted by length (everything
		 * NUM_VSIZES or longer is put in the last pool).  So
		 * we can resterict the sort to just those variables
		 * that match the length.  This is a surprisingly big
		 * speedup as there are many lookups during init.
		 */
		if (len >= NUM_VSIZES) {
			/* Scan all strings with len > NUM_VSIZES */
			v = findvar(cur->vars + cur->vsizes[NUM_VSIZES - 1],
			            cur->vars + cur->size, name);
		} else {
			/* Scan just the strings that match the len */
			v = findvar(cur->vars + cur->vsizes[len - 1],
			            cur->vars + cur->vsizes[len], name);
		}

		if (v) {
			return v;
		}
	}

#ifdef BCMSPACE
	v = findvar(defvars, defvars + DEFVARSLEN, name);
	if (v)
		NVR_MSG(("%s: variable %s defaulted to %s\n",
		         __FUNCTION__, name, v));
#endif	/* BCMSPACE */

	return v;
}

char *
nvram_get(const char *name)
{
	NVRAM_RECLAIM_CHECK(name);
	return nvram_get_internal(name);
}

int
BCMATTACHFN(nvram_set)(const char *name, const char *value)
{
	return 0;
}

int
BCMATTACHFN(nvram_unset)(const char *name)
{
	return 0;
}

int
BCMATTACHFN(nvram_reset)(void *si)
{
	return 0;
}

int
BCMATTACHFN(nvram_commit)(void)
{
	return 0;
}

static int
nvram_getall_internal(char *buf, int count)
{
	int len, resid = count;
	vars_t *this;

	this = vars;
	while (this) {
		char *from, *lim, *to;
		int acc;

		from = this->vars;
		lim = (char *)((uintptr)this->vars + this->size);
		to = buf;
		acc = 0;
		while ((from < lim) && (*from)) {
			len = strlen(from) + 1;
			if (resid < (acc + len))
				return BCME_BUFTOOSHORT;
			bcopy(from, to, len);
			acc += len;
			from += len;
			to += len;
		}

		resid -= acc;
		buf += acc;
		this = this->next;
	}
	if (resid < 1)
		return BCME_BUFTOOSHORT;
	*buf = '\0';
	return 0;
}

int
nvram_getall(char *buf, int count)
{
	NVRAM_RECLAIM_CHECK("nvram_getall");
	return nvram_getall_internal(buf, count);
}

#ifdef BCMQT
extern void nvram_printall(void);
/* QT: print nvram w/o a big buffer - helps w/memory consumption evaluation of USB bootloader */
void
nvram_printall(void)
{
	vars_t *this;

	this = vars;
	while (this) {
		char *from, *lim;

		from = this->vars;
		lim = (char *)((uintptr)this->vars + this->size);
		while ((from < lim) && (*from)) {
			printf("%s\n", from);
			from += strlen(from) + 1;
		}
		this = this->next;
	}
}
#endif /* BCMQT */
