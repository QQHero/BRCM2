/*
 本程序用于测验博通无线驱动的netlink交互接口
 Author : xiaowei1@tenda.cn
*/
#include "netlinkwl.h"
#include <stdio.h>
#include <string.h>
#include <netinet/in.h> 
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>

static int g_wlnetlink_fd =-1;


//read msg from wl
static void wlnetlink_read(int fd, unsigned char *buff, int bufflen)
{
    struct nlmsghdr *nlhdr;
    struct wlnetlink_msg_head *wlhdr;
    int payload_len;
    int ret;


    ret = read(fd, buff, bufflen);
    if (ret < NLMSG_HDRLEN) {
        printf("read error! ret=%d, errno=%d:%s\n", ret,
                    errno, strerror(errno));
        return;
    }

    //pharse netlink msg header
    nlhdr = (struct nlmsghdr *)buff;
    if (nlhdr->nlmsg_len > ret) {
        printf("wrong message length %d, read %d\n", nlhdr->nlmsg_len, ret);
        return;
    }

    payload_len = nlhdr->nlmsg_len - NLMSG_HDRLEN - sizeof(struct wlnetlink_msg_head);
    if (payload_len < 0) {
        printf("message is too short %d\n", nlhdr->nlmsg_len);
        return;
    }

    //pharse msg header
    wlhdr = (struct wlnetlink_msg_head *)NLMSG_DATA(buff);
    if (wlhdr->magicid != WLNETLINK_MSG_MAGIC) {
        printf("wrong magic.\n");
        return;
    }
    //pharse msg data
    printf("[slimehsiao]event %u get, length(%d) : %s\n", wlhdr->type, payload_len, wlhdr->msg);
}

static int wlnetlink_init()
{
    struct sockaddr_nl self_addr;
    int sockfd;

    sockfd = socket(PF_NETLINK, SOCK_RAW, KM_NETLINK_WL);
    if (sockfd < 0) {
        printf("create netlink socket failed.\n");
        return -1;
    }

    memset(&self_addr, 0, sizeof(self_addr));
    self_addr.nl_family = PF_NETLINK;
    self_addr.nl_pid = WLNETLINK_PID;
    self_addr.nl_groups = WLNETLINK_GROUP;

    if (bind(sockfd, (struct sockaddr*)&self_addr, sizeof(self_addr)) < 0) {
        printf("netlink bind failed, another instance may be running.\n");
        goto _bind_err;
    }
    //add_poll(sockfd, POLL_READ, wlnetlink_read NULL);
    return sockfd;
_bind_err:
    close(sockfd);
    return -1;
}

static void wlnetlink_exit()
{
    if (g_wlnetlink_fd > 0) {
        close(g_wlnetlink_fd);
        g_wlnetlink_fd =-1;
    }
    return;
}

int main(int argc, char *argv[])
{
    char buff[128];

    g_wlnetlink_fd  = wlnetlink_init();
    if (g_wlnetlink_fd <0 ) {
        printf("init error\n");
        return -1;
    }
    printf("[slimehsiao] wlnetlink init success\n");
    while (1) {
        wlnetlink_read(g_wlnetlink_fd, buff, sizeof(buff));
        sleep(10);
    }
     wlnetlink_exit();
    return 0;
}

