#include <linux/gpio.h>
#include "./../../../shared/opensource/include/bcm963xx/bcm_gpio.h"
#include "./../../../shared/opensource/include/bcm963xx/boardparms.h"
static DEFINE_SPINLOCK(tenda_gpio_lock);

extern void bcm_set_pinmux(unsigned int pin_num, unsigned int mux_num);
extern void bcm_led_driver_set(unsigned short num, unsigned short state);

#if defined(CONFIG_BCM96756)
#define MUX_NUM 4
#elif defined(CONFIG_BCM963178)
#define MUX_NUM 5
#endif

int bcm_gpio_request(struct gpio_chip *chip, unsigned int pin)
{
    unsigned long flags;

    spin_lock_irqsave(&tenda_gpio_lock, flags);
    /*6756芯片方案gpio对应的mux_num是4，而6750对应的是5*/
    bcm_set_pinmux(pin, MUX_NUM);
    spin_unlock_irqrestore(&tenda_gpio_lock, flags);
    return 0;
}

static int bcm_gpio_get_value(struct gpio_chip *chip, unsigned pin)
{
    int val;
    unsigned long flags;

    spin_lock_irqsave(&tenda_gpio_lock, flags);
    val = bcm_gpio_get_data(pin);
    spin_unlock_irqrestore(&tenda_gpio_lock, flags);
    return val;
}

static void bcm_gpio_set_value(struct gpio_chip *chip,
    unsigned pin, int value)
{
    unsigned long flags;

    spin_lock_irqsave(&tenda_gpio_lock, flags);
    bcm_led_driver_set(pin|BP_LED_USE_GPIO, value);
    spin_unlock_irqrestore(&tenda_gpio_lock, flags);
}

static int bcm_gpio_direction_input(struct gpio_chip *chip,
    unsigned int pin)
{
    unsigned long flags;

    spin_lock_irqsave(&tenda_gpio_lock, flags);
    bcm_gpio_set_dir(pin, 0);
    spin_unlock_irqrestore(&tenda_gpio_lock, flags);
    return 0;
}

static int bcm_gpio_direction_output(struct gpio_chip *chip,
    unsigned pin, int value)
{
    unsigned long flags;
        
    spin_lock_irqsave(&tenda_gpio_lock, flags);
    bcm_gpio_set_dir(pin, 1);
    spin_unlock_irqrestore(&tenda_gpio_lock, flags);
    return 0;
}

static int bcm_gpio_get_direction(struct gpio_chip *chip, unsigned pin)
{
    int val;
    unsigned long flags;

    spin_lock_irqsave(&tenda_gpio_lock, flags);
    val = bcm_gpio_get_dir(pin);
    spin_unlock_irqrestore(&tenda_gpio_lock, flags);
    return val;
}

static struct gpio_chip bcm_gpio_peripheral = {
    .label              = "bcm_gpio",
    .request            = bcm_gpio_request,
    .get                = bcm_gpio_get_value,
    .set                = bcm_gpio_set_value,
    .direction_input    = bcm_gpio_direction_input,
    .direction_output   = bcm_gpio_direction_output,
    .get_direction      = bcm_gpio_get_direction,
    .ngpio              = 87,
};

static int __init bcm_gpio_peripheral_init(void)
{
    int err;

    printk("BCM GPIO Platform controller driver init\n");
    err = gpiochip_add(&bcm_gpio_peripheral);
    if (err) {
        printk("cannot add bcm BSP_GPIO chip, error=%d", err);
        return err;
    }
    return 0;
}
arch_initcall(bcm_gpio_peripheral_init);