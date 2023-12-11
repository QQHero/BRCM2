#include <linux/module.h>
#include <linux/version.h>
#include <linux/proc_fs.h>

#include <linux/seq_file.h>
#include "td_wlan_proc.h"
#ifdef TD_EASYMESH_SUPPORT
#include "td_easymesh_dbg.h"
#endif

#include "td_debug.h"

#ifndef CONFIG_TENDA_PRIVATE_WIFI2
static char *s_proc_tenda_name = "td_wlan";
#else
static char *s_proc_tenda_name = "td_wlan2";
#endif
struct proc_dir_entry *s_proc_tenda_root = NULL;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
    void td_wlan_proc_exit(void)
    {
        return;
    }
    int td_wlan_proc_init(void)
    {
        return 0;
    }
#else
#define TENDA_DECLARE_READ_PROC_FOPS(read_proc) \
    int read_proc##_open(struct inode *inode, struct file *file) \
    { \
            return(single_open(file, read_proc, PDE_DATA(file_inode(file)))); \
    } \
    struct file_operations read_proc##_fops = { \
            .open            = read_proc##_open, \
            .read            = seq_read, \
            .llseek         = seq_lseek, \
            .release        = single_release, \
    }

#define TENDA_DECLARE_WRITE_PROC_FOPS(write_proc) \
    static ssize_t write_proc##_write(struct file * file, const char __user * userbuf, \
             size_t count, loff_t * off) \
    { \
        return write_proc(file, userbuf,count, PDE_DATA(file_inode(file))); \
    } \
    struct file_operations write_proc##_fops = { \
            .write            = write_proc##_write, \
    }


#define TENDA_DECLARE_READ_WRITE_PROC_FOPS(read_proc,write_proc) \
    static ssize_t read_proc##_write(struct file * file, const char __user * userbuf, \
             size_t count, loff_t * off) \
    { \
        return write_proc(file, userbuf,count, PDE_DATA(file_inode(file))); \
    } \
    int read_proc##_open(struct inode *inode, struct file *file) \
    { \
            return(single_open(file, read_proc, PDE_DATA(file_inode(file)))); \
    } \
    struct file_operations read_proc##_fops = { \
            .open            = read_proc##_open, \
            .read            = seq_read, \
            .write            = read_proc##_write, \
            .llseek         = seq_lseek, \
            .release        = single_release, \
    }


#define TENDA_CREATE_PROC_READ_ENTRY(name, func, data) \
        proc_create_data(name, 0644, s_proc_tenda_root, &func##_fops, (void *)data)

#define TENDA_CREATE_PROC_READ_WRITE_ENTRY(name, func, write_func, data) \
        proc_create_data(name, 0644, s_proc_tenda_root, &func##_fops, (void *)data)

#define TENDA_CREATE_PROC_WRITE_ENTRY(name, write_func, data) \
        proc_create_data(name, 0644, s_proc_tenda_root, &write_func##_fops, (void *)data)
#ifdef TD_EASYMESH_SUPPORT
TENDA_DECLARE_READ_WRITE_PROC_FOPS(td_emdbg_read_proc,td_emdbg_write_proc);
TENDA_DECLARE_READ_WRITE_PROC_FOPS(td_em_read_fake_proc,td_em_write_fake_proc);
#endif

TENDA_DECLARE_READ_WRITE_PROC_FOPS(td_debug_read_proc,td_debug_write_proc);
TENDA_DECLARE_READ_WRITE_PROC_FOPS(td_debug_filter_read_proc,td_debug_filter_write_proc);

int td_wlan_proc_init(void)
{
    printk("TENDA WLAN: init proc \n");

    s_proc_tenda_root = proc_mkdir(s_proc_tenda_name, NULL);
    if (!s_proc_tenda_root) {
        printk("%s proc initialization failed! \n", s_proc_tenda_name);
        return -1;
    }
#ifdef TD_EASYMESH_SUPPORT
    TENDA_CREATE_PROC_READ_WRITE_ENTRY("easymesh_dbg", td_emdbg_read_proc, td_emdbg_write_proc, NULL);
    TENDA_CREATE_PROC_READ_WRITE_ENTRY("easymesh_fake_data", td_em_read_fake_proc, td_em_write_fake_proc, NULL);
#endif

    TENDA_CREATE_PROC_READ_WRITE_ENTRY("debug", td_debug_read_proc, td_debug_write_proc, NULL);
    TENDA_CREATE_PROC_READ_WRITE_ENTRY("debug_filter", td_debug_filter_read_proc, td_debug_filter_write_proc, NULL);

    return 0;
}

void td_wlan_proc_exit(void)
{
    printk("TENDA WLAN: remove proc \n");
    /* remove proc entries */

#ifdef TD_EASYMESH_SUPPORT
    remove_proc_entry("easymesh_dbg", s_proc_tenda_root);
    remove_proc_entry("easymesh_fake_data", s_proc_tenda_root);
#endif
    remove_proc_entry("debug", s_proc_tenda_root);
    remove_proc_entry("debug_filter", s_proc_tenda_root);
    remove_proc_entry(s_proc_tenda_name,NULL);
    return;
}


#endif

