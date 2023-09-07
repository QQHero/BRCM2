/******************************************************************************
          版权所有 (C), 2020-2025, 深圳市吉祥腾达科技有限公司
 ******************************************************************************
  文 件 名   : holymsg.h
  版 本 号   : 初稿
  作    者   : platform
  生成日期   : 2021年3月27日
  最近修改   :
  功能描述   : 平台提供发布-订阅机制的公共接口头文件

  修改历史   :
  1.日    期   : 2021年2月21日
    作    者   : platform
    修改内容   : 创建文件

******************************************************************************/

#ifndef HOLYMSG_H
#define HOLYMSG_H

/*----------------------头文件包含区---------------------*/
#include <string.h>
#include <sys/epoll.h>
#include "common_cjson.h"

/*----------------------宏定义区-------------------------*/
#define HOLYMSG_INET_PORT 0x1234

/*----------------------函数声明区-----------------------*/
typedef void (*holymsg_handler_t)(char *type, void *data, unsigned len);
typedef void (*holymsg_handler_1to1_t)(char *type, void *data, unsigned len, int peerfd);
typedef void (*holytimer_handler_t)(void *arg);
typedef void (*holymsg_listener_t)(int fd, char event, void *private);
typedef void (*holymsg_server_listener_t)(int fd, char event, void *data, unsigned len);

/*
    holymsg_cm_subscribe - cm subscribe message
    @type: message type
    @handler: message handler

    returns: 1=OK, 0=FAIL
*/
int holymsg_cm_subscribe(char *type, holymsg_handler_t handler);
#define cm_subscribe(type, handler) holymsg_cm_subscribe(type, (holymsg_handler_t)handler)

/*
    holymsg_cm_unsubscribe - cm unsubscribe message
    @type: message type
    @handler: message handler

    returns: 1=OK, 0=FAIL
*/
int holymsg_cm_unsubscribe(char *type, holymsg_handler_t handler);
#define cm_unsubscribe(type, handler) holymsg_cm_unsubscribe(type, (holymsg_handler_t)handler)

/*
    holymsg_cm_inet_subscribe - cm inet subscribe message
    @type: message type
    @handler: message handler

    returns: 1=OK, 0=FAIL
*/
int holymsg_cm_inet_subscribe(char *type, holymsg_handler_t handler);
#define cm_inet_subscribe(type, handler) holymsg_cm_inet_subscribe(type, (holymsg_handler_t)handler)

/*
    holymsg_cm_inet_unsubscribe - cm inet unsubscribe message
    @type: message type
    @handler: message handler

    returns: 1=OK, 0=FAIL
*/
int holymsg_cm_inet_unsubscribe(char *type, holymsg_handler_t handler);
#define cm_inet_unsubscribe(type, handler) holymsg_cm_inet_unsubscribe(type, (holymsg_handler_t)handler)

/*
    holymsg_publish - publish message
    @type: message type
    @data: message data
    @len: length of @data

    returns: 1=OK, 0=FAIL
*/
int holymsg_publish(char *type, void *data, unsigned len);
#define publish(type, data, len) holymsg_publish(type, data, len)

/*
    holymsg_publish_and_respond - publish message and receive kernel respond message
    @type: message type
    @data: message data
    @len: length of @data
    @respond_data: respond data

    returns: 1=OK, 0=FAIL
*/
int holymsg_publish_and_respond(char *type, void *data, unsigned len, cJSON *respond_json);
#define publish_and_respond(type, data, len, respond_json) \
    holymsg_publish_and_respond(type, data, len, respond_json)

/* publish_value - publish message whose data is basic value (int/float/...) */
#define publish_value(type, value) ({\
        typeof(value) v = value;\
        publish(type, &v, sizeof(v));\
    })

/* publish_string - publish message whose data is string */
#define publish_string(type, fmt, args...) ({\
        char _data[1024] = {0};\
        if (fmt) {\
            snprintf(_data, sizeof(_data), fmt, ##args);\
        }\
        publish(type, _data, strlen(_data) + 1);\
    })

/* publish_empty - publish message that without data */
#define publish_empty(type) publish(type, NULL, 0)

/*
    holymsg_subscribe - subscribe message
    @type: message type
    @handler: message handler

    returns: 1=OK, 0=FAIL
*/
int holymsg_subscribe(char *type, holymsg_handler_t handler);
#define subscribe(type, handler) holymsg_subscribe(type, (holymsg_handler_t)handler)

/*
    holymsg_unsubscribe - unsubscribe message
    @type: message type
    @handler: message handler, NULL=all

    returns: 1=OK, 0=FAIL
*/
int holymsg_unsubscribe(char *type, holymsg_handler_t handler);
#define unsubscribe(type, handler) holymsg_unsubscribe(type, (holymsg_handler_t)handler)

/*
    holymsg_inet_publish - publish message (over internet)
    @type: message type
    @data: message data
    @len: length of @data

    returns: 1=OK, 0=FAIL
*/
int holymsg_inet_publish(char *type, void *data, unsigned len);
#define ipublish(type, data, len) holymsg_inet_publish(type, data, len)

/* send msg only once */
int holymsg_inet_publish_no_retry(char *type, void *data, unsigned len);
#define ipublish_no_retry(type, data, len) holymsg_inet_publish_no_retry(type, data, len)


/* ipublish_value - publish message (over internet) whose data is basic value (int/float/...) */
#define ipublish_value(type, value) ({\
        typeof(value) v = value;\
        ipublish(type, &v, sizeof(v));\
    })

/* ipublish_string - publish message (over internet) whose data is string */
#define ipublish_string(type, fmt, args...) ({\
        char _data[1024] = {0};\
        if (fmt) {\
            snprintf(_data, sizeof(_data), fmt, ##args);\
        }\
        ipublish(type, _data, strlen(_data) + 1);\
    })

/* ipublish_empty - publish message (over internet) that without data */
#define ipublish_empty(type) ipublish(type, NULL, 0)

/*
    holymsg_inet_subscribe - subscribe message (over internet)
    @type: message type
    @handler: message handler

    returns: 1=OK, 0=FAIL
*/
int holymsg_inet_subscribe(char *type, holymsg_handler_t handler);
#define isubscribe(type, handler) holymsg_inet_subscribe(type, (holymsg_handler_t)handler)

/*
    holymsg_inet_unsubscribe - unsubscribe message (over internet)
    @type: message type
    @handler: message handler, NULL=all

    returns: 1=OK, 0=FAIL
*/
int holymsg_inet_unsubscribe(char *type, holymsg_handler_t handler);
#define iunsubscribe(type, handler) holymsg_inet_unsubscribe(type, (holymsg_handler_t)handler)

/*
    holymsg_set_inet_master - set master ip for over LAN transmition
    @ip: master ip in host byte order, 0 means this host is the master
*/
void holymsg_set_inet_master(uint32_t ip);

/*
    holymsg_run - receive and process messages

    NOTE: run in current thread, this WILL block.

    returns: 1=OK, 0=FAIL
*/
int holymsg_run(void);
#define runmsg() holymsg_run()

/*
    holymsg_run_async - receive and process messages

    NOTE: run in a new thread, this will NOT block.

    returns: 1=OK, 0=FAIL
*/
int holymsg_run_async(void);
#define async_runmsg() holymsg_run_async()

/*
    holy_set_interval - start a timer
    @timeout: interval in seconds
    @handler: timer handler
    @arg: pass to handler

    return: timer object when seccuss, or NULL when failure
*/
void *holymsg_set_interval(double timeout, holytimer_handler_t handler, void *arg);
#define set_interval(timeout, handler, arg) \
    holymsg_set_interval(timeout, (holytimer_handler_t)handler, arg)

/*
    holymsg_set_timeout - start a One-Shot timer
    @timeout: interval in seconds
    @handler: timer handler
    @arg: pass to handler

    return: timer object when seccuss, or NULL when failure
*/
void *holymsg_set_timeout(double timeout, holytimer_handler_t handler, void *arg);
#define set_timeout(timeout, handler, arg) \
    holymsg_set_timeout(timeout, (holytimer_handler_t)(handler), (void *)(arg))

/*
    holymsg_kill_timer - stop a timer
    @timer: timer object returned by holymsg_set_interval/holymsg_set_timeout
*/
void holymsg_kill_timer(void *timer);
#define kill_timer holymsg_kill_timer

/*
    holymsg_add_listener - add a listner
    @fd: listen on this fd
    @event: can be "r", "w" or "rw"
    @listener: a callback handler when @event happens
    @private: private data

    return: 1=OK, 0=FAIL
*/
int holymsg_add_listener(int fd, char *event, holymsg_listener_t listener, void *private);

/*
    holymsg_del_listener - delete a listner
    @fd: the listened fd
*/
void holymsg_del_listener(int fd);

/*
    holymsg_add_server_loop_listener - creat a unix or inet server
    @path: unix or inet server configure
    @event: client read or write event
    @handler: a callback handler when @event happens

    return: >0=server fd, 0=FAIL
*/
int holymsg_add_server_loop_listener(char *path, char *event, holymsg_server_listener_t handler);

/*
    holymsg_del_server_loop_listener - delete a unix or inet server
    @server_fd: server fd
*/
void holymsg_del_server_loop_listener(int server_fd);

/*
    holymsg_client_send_msg - client send a message to unix or inet server
    @data: message data
    @data_len: message data len
    @path: unix or inet server configure

    return: >0=OK, 0=FAIL
*/
int holymsg_client_send_msg(void *data, int data_len, char *path);

/*
    holymsg_send_msg_by_fd - client send a message to sockfd
    @data: message data
    @data_len: message data len
    @sockfd: unix or inet socket fd

    return: >0=OK, 0=FAIL
*/
int holymsg_send_msg_by_fd(void *data, int data_len, int sockfd);

/*
    holymsg_client_rcv_msg - client rev a message from unix or inet server
    @data: message data
    @path: unix or inet server configure

    return: >0=message data len, 0=FAIL
*/
int holymsg_client_rcv_msg(void **data, char *path);

#endif // HOLYMSG_H
