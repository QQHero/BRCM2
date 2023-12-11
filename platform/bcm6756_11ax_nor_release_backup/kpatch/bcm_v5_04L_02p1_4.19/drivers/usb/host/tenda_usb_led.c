#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/acpi.h>

#include <linux/iof_drv.h>
#include <linux/usb/led.h>

struct usb_led_ctrl led_usb;
extern unsigned int usb_led_irq_cnt;
#define USB_LED_FRQ 5
#define USB_LED_FRQ_OFFSET 16
#define USB_LED_ID_OFFSET  8
#define IRQ_RECYCLE 0x3ffffff
static void _gpio_usb_led_fn(struct work_struct *w)
{
    struct usb_led_ctrl *usb_led;
    unsigned int usb_led_data = 0;
    unsigned int usb_led_stats = USB_LED_INITIAL;
    struct delayed_work *dw;

    dw = container_of(w, struct delayed_work, work);
    if (dw == NULL) {
        printk("%s dw is NULL\n", __func__);
        return;
    }

    usb_led = container_of(dw, struct usb_led_ctrl, dw);

    usb_led_data |= (usb_led->usb_led_id << USB_LED_ID_OFFSET);
    
    //没有读写中断或者读写终止
    if (usb_led_irq_cnt == usb_led->last_cnt) {
        usb_led_irq_cnt = 0;
        usb_led->last_cnt = 0;
        if (usb_led->conn_status) {
            usb_led_stats = USB_LED_ON;
        } else {
            usb_led_stats = USB_LED_OFF;
        }
    } else {
        if(usb_led_irq_cnt > IRQ_RECYCLE){
            usb_led_irq_cnt = 0;
        }
        usb_led->last_cnt = usb_led_irq_cnt;
        if (usb_led->conn_status) {
            usb_led_stats = USB_LED_BLINK;
        } else {
            usb_led_stats = USB_LED_OFF;
        }
    } 
    if (usb_led->last_status != usb_led_stats) {
        usb_led_data |= ((USB_LED_FRQ << USB_LED_FRQ_OFFSET) | usb_led_stats);
        iof_drv_notify_triggle(IOF_USB_EVENT, &usb_led_data);
        usb_led->last_status = usb_led_stats;
    }
    schedule_delayed_work(&(usb_led->dw), HZ/10);
    return;
}

static void usb_connect_state(unsigned int state)
{
    led_usb.conn_status = state;
}

static int custom_usb_led_probe(struct platform_device *dev)
{
    printk("init USB led probe\n");
    
    usb_led_irq_cnt = 0;
    usb_check = usb_connect_state;
    
    memset(&led_usb, 0, sizeof(led_usb));

    led_usb.usb_led_id = CONFIG_CUSTOM_USB_LED_INDEX;
    led_usb.last_status = USB_LED_INITIAL;

    INIT_DELAYED_WORK(&(led_usb.dw), _gpio_usb_led_fn);
    
    schedule_delayed_work(&(led_usb.dw), HZ/10);
    return 0;
}

static int custom_usb_led_driver_remove(struct platform_device *pdev)
{
    flush_delayed_work(&(led_usb.dw));
    cancel_delayed_work_sync(&(led_usb.dw));
    usb_led_irq_cnt = -1;
    usb_check = NULL;

    return 0;
}

static struct platform_device custom_usb_led_device = {
    .name            = "custom_usb_led",
    .id              =  0,
};

static struct platform_driver custom_usb_led_driver = {
    .probe    = custom_usb_led_probe,
    .remove   = custom_usb_led_driver_remove,
    .driver = {
         .name  = "custom_usb_led",
    },
};

static int __init custom_usb_led_init(void)
{
    int ret = 0;
    ret = platform_device_register(&custom_usb_led_device);
    ret = platform_driver_register(&custom_usb_led_driver);
    return ret;
}

static void custom_usb_led_exit(void)
{
    platform_driver_unregister(&custom_usb_led_driver);
}
module_init(custom_usb_led_init);
module_exit(custom_usb_led_exit);

MODULE_DESCRIPTION("USB Led Driver");
MODULE_LICENSE("GPL");

