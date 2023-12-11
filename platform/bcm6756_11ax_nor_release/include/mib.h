#ifndef MIB_H
#define MIB_H

#define MIB_TMP_STR_LEN                 256
#define CONFIG_CRC_SIZE                 sizeof(unsigned long)
#ifdef CONFIG_CFM_BACKUP
#define CONFIG_CFM_STATUS_SIZE          sizeof(unsigned long)
#else
#define CONFIG_CFM_STATUS_SIZE          0
#endif
#define NAND_FLASH_SIZE                 128 * 1024

//恢复出厂设置的配置文件所在路径
#define DEFAULT_MIB_FILE                "/webroot/default.cfg"
#define WIFI_MODULE_LOAD_MIB_NAME       "wifi.module.enable"
#define SSID_CHANGE_MIB_NAME            "ssid.change.enable"

void mib_init();
int GetCfmValue(char *Name, char *Value, int ValueLen);
int SetCfmValue(char *Name,char *Value);
int UnSetCfmValue(char *Name);
int SaveCfm2Flash(int type);
int ShowCfmValue(char *file);
void RestoreMTD(int def);

unsigned int mib_hash(void *data);
int mib_compare(void *d1, void *d2);
int crack(char *buf, char **key, char **val);
int file2buf(char *filename, char *data,int len);
int filesize(char *filename);
int mib_file_write(char *filename, char *buf, int len);

#endif

