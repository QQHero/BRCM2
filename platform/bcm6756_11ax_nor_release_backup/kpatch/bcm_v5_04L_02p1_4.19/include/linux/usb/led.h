#ifndef __LINUX_USB_LED_H
#define __LINUX_USB_LED_H

#define USB_LED_INITIAL 0
#define USB_LED_ON      1
#define USB_LED_OFF     2
#define USB_LED_BLINK   3 

#define USB_LINK   0x10
#define USB_UNLINK 0x20

typedef void (*usb_connet_checkout)(unsigned int);

struct usb_led_ctrl {
    int last_cnt;
    unsigned int usb_led_id; /*所使用的gpio_index号，不是gpio_id*/
    unsigned int last_status; /*保存上一次状态改变值*/
    unsigned int conn_status; /*保存当前hub触发的状态改变值*/
    struct delayed_work dw; /*工作队列使用，相关参数和初始化可参考工作队列的初始化说明*/
};
extern usb_connet_checkout usb_check;
#endif
