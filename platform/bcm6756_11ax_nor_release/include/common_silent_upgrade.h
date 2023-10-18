#ifndef __SILENT_UPGRADE_H__
#define __SILENT_UPGRADE_H__

#include "common_extern.h"

/*----------------------------------------------*
 * ºê¶¨Òå                                               *
 *----------------------------------------------*/
#define UPG_PATH         "/var/silent_upgrade"
#define MOD_PATH        "/lib/modules"
#define ORG_PATH        "/var"

#define CRC_SIZE            (4)
#define RET_MALLOC_ERR      (-4)
#define RET_NO_SPACE        (-3)
#define RET_CRC_ERR         (-2)
#define RET_FILE_ERR        (-1)
#define RET_OK              (0)

#define CRC_HEX2STR(val, array)         do {    \
        array[0] = (val >> 24) & 0xff;          \
        array[1] = (val >> 16) & 0xff;          \
        array[2] = (val >> 8) & 0xff;           \
        array[3] = val & 0xff;                  \
        } while (0)

#define CRC_STR2HEX(array, val)         do {    \
    val = ((array[0] & 0xff) << 24)             \
        | ((array[1] & 0xff) << 16)             \
        | ((array[2] & 0xff) << 8)              \
        | (array[3] & 0xff);                    \
        } while (0)


int crc_set(const char *filename);
int silent_upgrade_erase_mtd(char *mtdname);
int silent_upgrade_read(const char *pathname, unsigned char * buff, unsigned int filesize);
int silent_upgrade_write (const char *pathname, unsigned char * buff, int len);

void silent_upgrade_show(void);
void silent_upgrade_usage(void);
int silent_upgrade_get(const char *filename);
int silent_upgrade_save(const char *filename);
int silent_upgrade_remove(const char *filename);
int silent_upgrade_check(const char *filename);

#endif
