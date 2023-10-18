#ifndef COMMON_FLASH_H
#define COMMON_FLASH_H
#include <unistd.h>
#include "common_extern.h"

//页面错误提示使用，不同文件升级错误起始号
#define UPGRADE_FW_ERR_BASE         1000
#define UPGRADE_CFG_ERR_BASE        2000
#define UPGRADE_POLICY_ERR_BASE     3000
#define UPGRADE_IMAGE_ERR_BASE      4000
#define min(x,y)  ({ typeof(x) _x = (x); typeof(y) _y = (y); _x < _y ? _x : _y; })

#define IH_MAGIC    0x27051956    /* Image Magic Number*/
#define IP_MAGIC    0x27051958    /* Policy Magic Number*/
#define DUAL_MAGIC  0x476f6431    /*dual image Magic Number*/
#define BOOT_MAGIC  0x48844884    /* boot Magic Number*/

//配置文件升级错误号，同UPGRADE_CFG_ERR_BASE一起使用
typedef enum SYS_CFG_RET
{
     CFG_UPGRADE_OK     = 0,
     CFG_UPLOAD_ERR     = -1,
     CFG_FILE_INVALID   = -2
}SYS_CFG_RET_ENUM;

//固件升级错误号，同UPGRADE_FW_ERR_BASE一起使用
typedef enum FW_RET
{
    FW_VALID_IMAGE      = 0,
    FW_INVALID_FORMAT   = -1,
    FW_INVALID_CRC      = -2,
    FW_INVALID_SIZE     = -3,
    FW_UPLOAD_ERROR     = -4,
    FW_NO_MEMORY        = -5
}FW_RET_ENUM;

//升级类型，用于区分不同的升级类型
typedef enum UPGRADE_TYPE
{
    UPGRADE_FW      = 0,
    UPGRADE_POLICY,
    UPGRADE_CFG,
    UPGRADE_BOOT
}UPGRADE_TYPE_ENUM;

typedef struct MTD_USER_INFO
{
    u8  type;
    u32 flags;
    u32 size;     // Total size of the MTD
    u32 erasesize;
    u32 oobblock;  // Size of OOB blocks (e.g. 512)
    u32 oobsize;   // Amount of OOB data per block (e.g. 16)
    u32 ecctype;
    u32 eccsize;
}MTD_USER_INFO_STRU;

typedef struct ERASE_USER_INFO
{
    u32 start;
    u32 length;
}ERASE_USER_INFO_STRU;
#if defined(BCM96756) && !defined(CONFIG_TINY_NORFLASH_SURPPORT)
/**
 * struct mtd_ecc_stats - error correction stats
 *
 * @corrected:  number of corrected bits
 * @failed:     number of uncorrectable errors
 * @badblocks:  number of bad blocks in this partition
 * @bbtblocks:  number of blocks reserved for bad block tables
 */
typedef struct MTD_USER_ECC_STATS {
    unsigned int corrected;
    unsigned int failed;
    unsigned int badblocks;
    unsigned int bbtblocks;
}MTD_USER_ECC_STATS_STRU;
/* Check if an eraseblock is bad */
#define MEM_GETBADBLOCK          _IOW('M', 11, long long)
/* Get statistics about corrected/uncorrected errors */
#define ECC_GETSTATS             _IOR('M', 18, MTD_USER_ECC_STATS_STRU)
#endif
#define MEM_GET_INFO             _IOR('M', 1, MTD_USER_INFO_STRU)
#define MEM_ERASE                _IOW('M', 2, ERASE_USER_INFO_STRU)
#define MEMERASEBADBLKOFFS       _IOR('M', 24, ERASE_USER_INFO_STRU)
#if defined(CONFIG_FLASH_LOCK)
/* Lock a chip (for MTD that supports it) */
#define MEM_LOCK                  _IOW('M', 5, ERASE_USER_INFO_STRU)
/* Unlock a chip (for MTD that supports it) */
#define MEM_UNLOCK                _IOW('M', 6, ERASE_USER_INFO_STRU)
#endif

/*
 *FW type
 */
#define FW_TYPE_IMAGE_NULL          0x0
#define FW_TYPE_KERNEL_IMAGE        0x2
#define FW_TYPE_POLICY_IMAGE        0x4
#define FW_TYPE_BOOT_IMAGE          0x6 /*混合镜像里面带有boot*/
/*
 * all data in network byte order (aka natural aka bigendian)
 */
#define IH_NMLEN        16    /* Image Name Length        */

typedef struct image_header {
    u32    ih_magic;    /* Image Header Magic Number    */
    u32    ih_hcrc;    /* Image Header CRC Checksum    */
    u32    ih_time;    /* Image Creation Timestamp    */
    u32    ih_size;    /* Image Data Size        */
    u32    ih_load;    /* Data     Load  Address        */
    u32    ih_ep;        /* Entry Point Address        */
    u32    ih_dcrc;    /* Image Data CRC Checksum    */
    u8     ih_os;        /* Operating System        */
    u8     ih_arch;    /* CPU architecture        */
    u8     ih_type;    /* Image Type            */
    u8     ih_comp;    /* Compression Type        */
    u8     fw_cls;
    u8     rar5_flg;
    u8     nouse[2];
    u32    product_id;
    u32    kernel_size;
    u32    web_size;
    u8     ih_name[IH_NMLEN];    /* Image Name        */
} image_header_t;

typedef struct web_header {
    u32    iw_magic;    /* web Header Magic Number    */
    u32    iw_hcrc;    /* web Header CRC Checksum    */
    u32    iw_time;    /* web Creation Timestamp    */
    u32    iw_size;    /* web Data Size        */
    u32    iw_dcrc;    /* web Data CRC Checksum    */
    u8     fw_cls;
    u8     nouse[3];
    u32    product_id;
    u32    i_nouse;    /* not used*/
} web_header_t;


//策略升级错误号，同UPGRADE_POLICY_ERR_BASE一起使用
#define POLICY_UPGRADE_OK           0
#define POLICY_UPLOAD_ERR           -1
#define POLICY_FILE_INVALID         -2
#define POLICY_FILE_OLD             -3
#define POLICY_WFLASH_FAILD         -4

#define PICTURE_UPGRADE_OK          0
#define PICTURE_UPLOAD_ERR          -1
#define PICTURE_FILE_INVALID        -2
#define PICTURE_FILE_FULL           -3
#define PICTURE_FILE_LARGE          -4
#define NAND_FLASH_TYPE             4
#define HARD_DISK_TYPE              5


#define PICTURE_PATH                "/webroot/images/webpush/"
#define PICTURE_DEFAULT_PATH        "/webroot/images/default/"

#define WEB_PICTURE_PATH            "/var/webpicture.tar"
#define PICTURE_PATH_TURE           "webroot/images/webpush/"
#define PICTURE_NANDFLASH_PATH      "/cfg/webpush"

#define AC_SQL_PATH                 "/cfg/l_brcmnand/Dcg_database.db"
#define AC_POLICY_PATH              "/cfg/s_brcmnand/Dcg_database.db"

#define WEBFS_BZ2_PATH              "/var/webfs.bz2"
#define WEBFS_LZMA_PATH             "/var/webfs.lzma"
#define WEBFS_PK_PATH               "/var/webfs"
#define FW_MTD_CFM_NAME             "CFM"
#define NAND_MIB_FILE               "/cfg/mib.cfg"
#ifdef CONFIG_CFM_BACKUP
#ifdef CONFIG_LINUX_X86
#define FW_MTD_CFM_BACKUP_NAME      "/cfg_bak"
#else
#define FW_MTD_CFM_BACKUP_NAME      "CFM_BACKUP"
#endif
#define NAND_MIB_BACKUP_FILE        "/cfg_bak/mib_backup.cfg"
#endif
#define FW_MTD_CTCAPD_NAME          "ctcapd"
#define FW_MTD_PICTURE_NAME         "picture"
#define FW_MTD_CFM_URL_NAME         "CFM_URL"
#define FW_MTD_POLICY_NAME          "Policy"
#define NAND_FLASH_POLICY_FILE      "/cfg/policy.cfg"

#ifdef CONFIG_BCM_FLASHER
#define FW_MTD_NAME                 "image"
#elif defined(CONFIG_SF_UPGRADE)
#define FW_MTD_NAME                 "firmware"
#else
#define FW_MTD_NAME                 "KernelFS"
#endif
#define FW_MTD_NAME1                "KernelFS1"
#define FW_MTD_NAME2                "KernelFS2"
#define IMAGE_BOOT                  "image_boot"
#define IMAGE_FIRST_OFFSET          "image_first_offset"
#define IMAGE_SECOND_OFFSET         "image_second_offset"

#define POLICY_FILE                 "/etc/policy.cfg"
#define POLICY_BAK_FILE             "/etc/policy_bak.cfg"

#define FW_MTD_ALL                  "ALL"
#ifdef QUALCOMM
    #define FW_MTD_BOOT_NAME        "boot"
    //高通方案获取boot 版本文件路径
    #define CMDLINE_FILE "/proc/cmdline"
    #define VERSION_NAME "Version="
#else
    #define FW_MTD_BOOT_NAME        "Bootloader"
#endif
#define FW_MTD_NVRAM_NAME           "nvram"

#define FLASH_BLOCK_SIZE            1024 * 64
#define FW_KERNEL_TMP_PATH          "/var/fw.bin"
#define FW_WEB_TMP_PATH             "/var/web.bin"

#define FLASH_ONE_BLOCK_SIZE        64*1024

//策略文件读写FLASH
#define POLICY_BLOCK_SIZE           64*1024
#define POLICY_DATA_SIZE            POLICY_BLOCK_SIZE - sizeof(policy_blk_cfg_t)

typedef struct policy_blk_cfg
{
    u8 mac[UGW_MAC_BIN_LEN];
    u8 cnouse[16-UGW_MAC_BIN_LEN];
    u32  dcrc;
    u32  dlen;
    u32  comlen;
    u32  inouse;
} policy_blk_cfg_t;

#if defined(CONFIG_FLASH_PARTITION_LOCK)
/*****************************************************************************
 Prototype    : mtd_partitions_unlock
 Description  : flash分区解锁
 Input        : char *mtdname
 Output       : None
 Return Value : 0 : successs
                -1 : failed

  History        :
  1.Date         : 2021/5/14
    Author       : shimengmeng
    Modification : Created function

*****************************************************************************/
int mtd_partitions_unlock(const char *mtdname);

/*****************************************************************************
 Prototype    : mtd_partitions_lock
 Description  : flash分区加锁
 Input        : char *mtdname
 Output       : None
 Return Value : 0 : successs
                -1 : failed

  History        :
  1.Date         : 2021/5/14
    Author       : shimengmeng
    Modification : Created function

*****************************************************************************/
int mtd_partitions_lock(const char *mtdname);
#endif


/*****************************************************************************
 函 数 名  : mtd_open
 功能描述  : 打开指定的flash分区块设备
 输入参数  : const char *name
             int flags
 输出参数  : 无
 返 回 值  :
             <0：打开设备失败
             >0：打开设备成功，返回设备描述符

 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int mtd_open(const char *name, int flags);

/*****************************************************************************
 函 数 名  : erase_mtd
 功能描述  : 擦除指定的flash分区
 输入参数  : char *mtdname
 输出参数  : 无
 返 回 值  : 1:擦除成功
             0:擦除失败

 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int erase_mtd(const char *mtdname);

/*****************************************************************************
 函 数 名  : flash_read
 功能描述  : 从指定的flash分区中获取数据，从from位置开始读
 输入参数  : const char *mtd_name
             char *buf
             off_t from
             size_t len
 输出参数  : 无
 返 回 值  :
             <0：读取数据失败
             >0：返回实际读取到的数据长度

 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int flash_read(const char *mtd_name,char *buf, off_t from, size_t len);

/*****************************************************************************
 函 数 名  : flash_write
 功能描述  : 把数据写到指定的flash分区中，从to位置开始写
 输入参数  : const char *mtd_name
             char *buf
             off_t to
             size_t len
 输出参数  : 无
 返 回 值  :
             <0：写进指定flash分区失败
             >0：写进指定flash分区成功

 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int flash_write(const char *mtd_name,char *buf, off_t to, size_t len);

/*****************************************************************************
 函 数 名  : flash_upgradefw
 功能描述  : 把升级文件写进flash中，进行软件升级
 输入参数  : char *firmware
             int fw_tp
 输出参数  : 无
 返 回 值  :
             -1：升级固件失败
              1：升级固件成功

 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int flash_upgradefw(char *firmware, int fw_tp);
int write_mac_to_flash(char *mtdname, unsigned char *mac);
int get_mac_from_flash(char *mtdname, unsigned char *mac);
int get_r5flag_from_flash();

/*****************************************************************************
 函 数 名  : get_mtd_size
 功能描述  : 获取指定flash分区的大小
 输入参数  : char *mtdname
 输出参数  : 无
 返 回 值  :
             -1：获取分区大小失败
             >0：获取分区大小成功，并返回实际大小

 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int get_mtd_size(char *mtdname);

/*****************************************************************************
 函 数 名  : get_flash_size
 功能描述  : 获取flash的大小
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
              0：获取flash大小失败
             >0：获取flash大小成功，并返回实际大小

 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
int get_flash_size();

/*****************************************************************************
 函 数 名  : get_flash_type
 功能描述  : 获取flash类型
 输入参数  : 无
 输出参数  : 无
 返 回 值  : flash 类型，nand类型号为4，其他型号未做区分

 修改历史      :
  1.日    期   : 2016年4月19日
    作    者   : xujun
    修改内容   : 新生成函数

*****************************************************************************/
int get_flash_type();

/*****************************************************************************
 函 数 名  : get_mtd_num
 功能描述  : 获取mtd分区号
 输入参数  : char *name
 输出参数  : 无
 返 回 值  :

 修改历史      :
  1.日    期   : 2016年5月16日
    作    者   : xujun
    修改内容   : 新生成函数

*****************************************************************************/
int get_mtd_num(char *name);


/*****************************************************************************
 函 数 名  : nandfile_and_file_to_switch
 功能描述  : nandflash中的文件和内存中的文件之间转换
 输入参数  : char *SrcFile  源文件
             char *DstFile  目的文件
 输出参数  : 无
 返 回 值  : 成功:UGW_OK
             失败:UGW_ERR

 修改历史      :
  1.日    期   : 2016年5月17日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM nandfile_and_file_to_switch(char *SrcFile, char *DstFile);

/*****************************************************************************
 函 数 名  : flash_to_file
 功能描述  : 把flash中的数据写到指定的文件中
 输入参数  : char *filename
             char *mtdname
 输出参数  : 无
 返 回 值  :
             UGW_ERR：失败
             UGW_OK ：成功
 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM flash_to_file(char *filename, char *mtdname);

/*****************************************************************************
 函 数 名  : file_to_flash
 功能描述  : 把文件中的数据写到指定的flash分区中
 输入参数  : char *filename
             char *mtdname
 输出参数  : 无
 返 回 值  :
             UGW_ERR：失败
             UGW_OK ：成功

 修改历史      :
  1.日    期   : 2016年1月7日
    作    者   : zzh
    修改内容   : 新生成函数

*****************************************************************************/
UGW_RETURN_CODE_ENUM file_to_flash(char *filename,char *mtdname);

/*
     写 Flash,
    升级kernel, 升级webfs
*/
int flash_write_mtd(char *firmware, char *mtdname);

/*
    检查升级软件类型
*/
int check_fw_type(char *filebuf, long size);

/*
    crc 校验
*/
int check_fw_crc(int fw_type, char *filebuf, long size);

/*
    检测升级文件 productid
*/
int check_fw_valid(int fw_type, char *filename);

/*
    检测升级文件是否过大
*/
int check_mtd_size(int fw_type, char *filename);

/* *******************************************************
函数功能: 把指定文件内容写入指定的flash中
输入参数:
           mtdname  :     指定分区名
           mtd_offset:     写入分区偏移
           filename   :     文件名
           file_offset :     需写入文件内容偏移
           write_len  :     写入长度
返回值:     0  :   成功
               负值:   失败
*********************************************************/
int flash_write_mtd_offset(char *mtdname, unsigned mtd_offset, char *filename,
    unsigned int file_offset, unsigned int write_len);

/* *******************************************************
函数功能: 获取要升级的分区名
输入参数: mtd_name: 空间指针
            len : 空间长度（要大于FW_MTD_NAME1的长度 一般为UGW_TMP_STR_LEN）
返回值:
                   0：成功
               负值:   失败
*********************************************************/
int flash_get_target_mtd(char *mtd_name, unsigned int len);

/*****************************************************************************
 函 数 名  : mtd_mount_ubifs
 功能描述  : 挂载ubifs
 输入参数  : 
    name：分区名称，例如"CFG"
    ubifs_path：已经在文件系统中创建的带绝对路径的目录名，例如"/cfg"
 输出参数  : 0-失败 1-成功
 返 回 值  : int
*****************************************************************************/
int mtd_mount_ubifs(char *name, char* ubifs_path);
#endif

