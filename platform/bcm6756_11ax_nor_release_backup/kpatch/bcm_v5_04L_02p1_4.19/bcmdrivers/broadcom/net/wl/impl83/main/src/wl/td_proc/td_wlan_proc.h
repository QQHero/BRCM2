#ifndef _TENDA_WLAN_PROC_H_
#define _TENDA_WLAN_PROC_H_

#include <linux/proc_fs.h>
#include <linux/seq_file.h>

int td_wlan_proc_init(void);
void td_wlan_proc_exit(void);

int td_debug_read_proc(struct seq_file *s, void *data);
int td_debug_write_proc(struct file *file, const char *buffer,
                                                    unsigned long count, void *data);

int td_debug_filter_read_proc(struct seq_file *s, void *data);
int td_debug_filter_write_proc(struct file *file, const char *buffer,
                                                    unsigned long count, void *data);

extern struct proc_dir_entry *s_proc_tenda_root;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,9,0)
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,19)
static inline struct inode *file_inode(struct file *f)
{
    return f->f_path.dentry->d_inode;
}
#else // <= Linux 2.6.19
static inline struct inode *file_inode(struct file *f)
{
    return f->f_dentry->d_inode;
}
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
void *PDE_DATA(const struct inode *inode)
{
    return PDE(inode)->data;
}
#endif


#define TD_DECLARE_READ_PROC_FOPS(read_proc) \
int read_proc##_open(struct inode *inode, struct file *file) \
{ \
        return(single_open(file, read_proc, PDE_DATA(file_inode(file)))); \
} \
struct file_operations read_proc##_fops = { \
        .open           = read_proc##_open, \
        .read           = seq_read, \
        .llseek         = seq_lseek, \
        .release        = single_release, \
}

#define TD_DECLARE_WRITE_PROC_FOPS(write_proc) \
static ssize_t write_proc##_write(struct file * file, const char __user * userbuf, \
         size_t count, loff_t * off) \
{ \
    return write_proc(file, userbuf,count, PDE_DATA(file_inode(file))); \
} \
struct file_operations write_proc##_fops = { \
        .write          = write_proc##_write, \
}


#define TD_DECLARE_READ_WRITE_PROC_FOPS(read_proc,write_proc) \
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
        .open           = read_proc##_open, \
        .read           = seq_read, \
        .write          = read_proc##_write, \
        .llseek         = seq_lseek, \
        .release        = single_release, \
}

#define TD_CREATE_PROC_READ_ENTRY(name, func, data) \
    proc_create_data(name, 0644, s_proc_tenda_root, &func##_fops, (void *)data)

#define TD_CREATE_PROC_READ_WRITE_ENTRY(name, func, write_func, data) \
    proc_create_data(name, 0644, s_proc_tenda_root, &func##_fops, (void *)data)

#define TD_CREATE_PROC_WRITE_ENTRY(name, write_func, data) \
    proc_create_data(name, 0644, s_proc_tenda_root, &write_func##_fops, (void *)data)

#define TD_REMOVE_PROC(name)    \
    remove_proc_entry(name, s_proc_tenda_root)

#endif
