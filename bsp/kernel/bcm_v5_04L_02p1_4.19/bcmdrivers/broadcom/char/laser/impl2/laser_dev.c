/*---------------------------------------------------------------------------

<:copyright-BRCM:2013:proprietary:standard 

   Copyright (c) 2013 Broadcom 
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
 ------------------------------------------------------------------------- */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <laser.h>
#include <bcmsfp.h>
#include <bcmsfp_i2c.h>
#include "opticaldet.h"
#include <wan_drv.h>
extern struct device *bcm_i2c_legacy_sfp_get_dev(int bus);
extern int trxbus_is_pmd(int bus);

static int bus;

/* character major device number */
#define LASER_DEV_MAJOR   314
#define LASER_DEV_CLASS   "laser_dev"

static int Laser_Dev_File_Open(struct inode *inode, struct file *file) { return 0; }
static int Laser_Dev_File_Release(struct inode *inode, struct file *file) { return 0; }

static long Laser_Dev_File_Ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long val;
    uint32_t val32;
    void __user *argp = (void __user *)arg;
    enum bcmsfp_mon_attr op;
    int ret;
    struct device *wan_opt;

    if (!(wan_opt = bcm_i2c_legacy_sfp_get_dev(bus)))
        return -ENODEV;

    switch (cmd) 
    {
        case LASER_IOCTL_GET_RX_PWR:
            op = bcmsfp_mon_rx_power;
            break;
        case LASER_IOCTL_GET_TX_PWR:
            op = bcmsfp_mon_tx_power;
            break;
        case LASER_IOCTL_GET_TEMPTURE:
            op = bcmsfp_mon_temp;
            break;
        case LASER_IOCTL_GET_VOLTAGE:
            op = bcmsfp_mon_vcc;
            break;
        case LASER_IOCTL_GET_BIAS_CURRENT:
            op = bcmsfp_mon_bias_current;
            break;
        case LASER_IOCTL_GET_DRV_INFO:
            {
                int v_pn_len;
                char *v_pn;

                sfp_mon_read_buf(wan_opt, bcmsfp_mon_id_vendor_pn, 0, &v_pn, &v_pn_len);
                val32 = strncmp(v_pn, SMTC_VENDOR_PN, v_pn_len) ? BCM_I2C_PON_OPTICS_TYPE_LEGACY : BCM_I2C_PON_OPTICS_TYPE_SMTC;
                ret = 0;
                goto copy_exit;
            }
        default:
            printk("\nLaser_Dev: operation not supported\n" );
            return -EOPNOTSUPP;
    }

    ret = sfp_mon_read(wan_opt, op, 0, &val);
    val32 = val;

copy_exit:
    if (!ret && copy_to_user(argp, &val32, sizeof(val32)))
        return -EFAULT;

    return ret;
}

static const struct file_operations laser_file_ops = {
    .owner = THIS_MODULE,
    .open = Laser_Dev_File_Open,
    .release = Laser_Dev_File_Release,
    .unlocked_ioctl = Laser_Dev_File_Ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = Laser_Dev_File_Ioctl,
#endif
};

static int Laser_Dev_Init(void)
{
    int ret;
    bus = wantop_bus_get();

    if (bus < 0)
    {
        printk("\nLaser_Dev: optical module not present, not loading!\n");
        return 0;
    }

    /* Silently do nothing if bus is PMD */
    if (trxbus_is_pmd(bus))
        return 0;

    ret = register_chrdev(LASER_DEV_MAJOR, LASER_DEV_CLASS, &laser_file_ops);
    if (ret < 0)
    {
        printk(KERN_ERR "%s: can't register major %d\n",LASER_DEV_CLASS, LASER_DEV_MAJOR);
        return ret;
    }

    return 0;
}

static void Laser_Dev_Exit(void)
{
    unregister_chrdev(LASER_DEV_MAJOR, LASER_DEV_CLASS);
}

module_init(Laser_Dev_Init);
module_exit(Laser_Dev_Exit);

MODULE_DESCRIPTION("Laser Device driver wrapper for bcmsfp");
MODULE_LICENSE("Proprietary");

