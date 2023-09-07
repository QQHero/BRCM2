/******************************************************************************
          版权所有 (C), 2015-2019, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  版 本 号   : 1.0
  作    者   : Sunjiajun
  生成日期   : 2019年12月
  最近修改   :

  功能描述   : 本文件内存放应用层(steerd), 本模块(td_multiap_steer)和原厂相关源文件
            均需要包含的公共定义.
            应用层会通过 td_maps_api.h 间接包含此头文件. 
            本模块和原厂相关源文件会通过 td_multiap_steer.h 间接包含此头文件.
******************************************************************************/
#ifndef _TD_MAPS_PUBLIC_H_
#define _TD_MAPS_PUBLIC_H_

#if defined(CONFIG_RTL8192CD) || defined(REALTEK)
/* QCA don't neeed it */
#define MAPS_IOCTL 0x900E

#elif defined(BROADCOM)
#define MAPS_IOCTL 332
#endif

typedef enum maps_log_level {
    MAPS_LOG_ERROR, 
    MAPS_LOG_INFO, 
    MAPS_LOG_DEBUG, 
    MAPS_LOG_DUMP, 
    MAPS_LOG_MAX
} maps_log_level_e;


#endif /* _TD_MAPS_PUBLIC_H_ */
