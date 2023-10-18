#ifndef _TD_EASYMESH_NL_H_
#define _TD_EASYMESH_NL_H_

void td_em_nl_exit(void);

int td_em_nl_init(void);

typedef enum netlink_user_type
{
    TD_EM_NLUSER_MULTICAST_MESH = 0, //组播给easymesh+Xmesh
    TD_EM_NLUSER_EASYMESH       = 1, //单播给easymesh进程
    TD_EM_NLUSER_WSERVER        = 2, //单播给wserver进程
    TD_EM_NLUSER_XMESH          = 3
}netlink_user_type_e;

void td_em_nl_send(char *data, int data_len, netlink_user_type_e type);

#endif
