/******************************************************************************
          版权所有 (C), 2015-2018, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : ugw_platform.h
  版 本 号   : 初稿
  作    者   : l0018898
  生成日期   : 2015年10月28日
  最近修改   :
  功能描述   : 

  功能描述   : UGW系统支撑平台对外（service/product）发布的唯一头文件

  修改历史   :
  1.日    期   : 2015年10月28日
    作    者   : l0018898
    修改内容   : 创建文件

******************************************************************************/
#ifndef UGW_PLATFORM_H
#define UGW_PLATFORM_H

// 公共头文件包含
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#ifdef CONFIG_LIBC_MUSL
#include <linux/sysinfo.h>
#else
#include <sys/sysinfo.h>
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "iof_lib_drv.h"
#include "common_extern.h"
#include "ugw_socket.h"
#include "cfm_extern.h"
#include "nvram_extern.h"
#include "log_extern.h"
#include "monitor_extern.h"
#include "common_string.h"
#include "common_flash.h"
#include "common_ifaddrs.h"
#include "common_list.h"
#include "common_proc.h"
#include "common_file.h"
#include "common_check.h"
#include "common_system.h"
#include "common_cjson.h"
#endif


