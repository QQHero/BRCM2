#ifndef _IOF_DRV_H_
#define _IOF_DRV_H_

typedef enum {
    IOF_USB_EVENT = 1, /* USB事件 */
    IOF_LSW_EVENT = 2, /* SWITCH事件 */
    IOF_WL_EVENT  = 3, /* 无线事件 */

    IOF_INVALID_EVENT = 32,
} iof_event_t;

typedef int (*trigger_hook_t)(iof_event_t, void *);

extern int __must_check iof_drv_notify_sync(iof_event_t event, 
    trigger_hook_t hook);
extern int __must_check iof_drv_notify_triggle(iof_event_t event, void *val);

#endif

