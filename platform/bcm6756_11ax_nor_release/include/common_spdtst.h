
#ifndef _COMMON_SPDTST_H
#define _COMMON_SPDTST_H
#include <common_extern.h>

#define SPDTST_MODULE       "spdtst"
struct spdtst_ping_desc {
    char *ip;       //dest ip
    int count;      //howmany ping times
};

/*
*send igmp ping to dest ip
*INPUT
* - desc
*       ip      : dest ip
*       count : how many igmp ping pack will send
*OUT
* - rtt         : output the avg rrt time(millisecond) is success
*RETURN
* UGW_OK  :  success
* others      :  failure
*/
extern UGW_RETURN_CODE_ENUM spdtst_ping(struct spdtst_ping_desc *desc, float *rtt);

/*
*Start a new process for internet speed test  
*RETURN
* UGW_OK  :  success
* others      :  failure
*/
extern UGW_RETURN_CODE_ENUM spdtst_internet_start_test (void);

/*
*get internet speed test's result
*OUTPUT
* - dlspeed : download speed of internet (kbps)
* -upspeed : upload speed of internet (kbps)
*RETURN
* UGW_OK  :  success
* others      :  failure
*/
extern UGW_RETURN_CODE_ENUM spdtst_internet_get_speed(int *dlspeed, int *upspeed);

extern UGW_RETURN_CODE_ENUM spdtst_internet_stop_test (void);


#endif
