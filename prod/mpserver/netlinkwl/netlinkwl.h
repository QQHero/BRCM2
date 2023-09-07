
#ifndef _NETLINKWL_H_
#define _NETLINKWL_H_

#define WLNETLINK_PID 6666
#define WLNETLINK_GROUP     2
#define KM_NETLINK_WL       96

#define WLNETLINK_BUFF_SIZE         128
#define WLNETLINK_MSG_MAGIC         0xfafafaa5

struct wlnetlink_msg_head {
    unsigned int magicid;
    unsigned char type;
    unsigned char msg[0];
};

#endif

