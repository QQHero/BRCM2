#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/errno.h>

#include <linux/iof_drv.h>

#define COUNTOF(ary) ((int) (sizeof(ary) / sizeof((ary)[0])))

struct iof_notify {
    iof_event_t event;
    trigger_hook_t triggle;
};

DEFINE_MUTEX(iof_mutex);

static struct iof_notify _notify [] = {
    {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, 
    {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, 
    {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, 
    {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, 
    {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, 
};

static inline struct iof_notify *iof_notify_get(uint32_t event)
{
    return &_notify[event];
}

int iof_drv_notify_sync(iof_event_t event, trigger_hook_t hook)
{
    int i;
    int rv;
    struct iof_notify *notify;

    rv = -ENOMEM;
    notify = iof_notify_get(0);
    for (i = 0; i < COUNTOF(_notify); i++) {
        if (notify->event) {
            notify++;
            continue;
        }
        
        notify->event = event;
        notify->triggle = hook;
        rv = 0;
        break;
    }

    return rv;
}
EXPORT_SYMBOL(iof_drv_notify_sync);

int iof_drv_notify_triggle(iof_event_t event, void *val)
{
    int i;
    int rv;
    struct iof_notify *notify;

    notify = iof_notify_get(0);

    for (i = 0; i < COUNTOF(_notify); i++) {
        if (notify->event == event) {
            break;
        }
        notify++;
    }
    
    if (notify->event == 0) {
        return -ENOENT;
    }

    if (!mutex_trylock(&iof_mutex)) {
        return -EBUSY;
    }
    
    rv = notify->triggle(event, val);
    
    mutex_unlock(&iof_mutex);
    
    return rv;
}
EXPORT_SYMBOL(iof_drv_notify_triggle);

