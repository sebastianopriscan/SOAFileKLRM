#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/printk.h>      
#include <linux/ptrace.h>       
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/fdtable.h>
#include <linux/fs_struct.h>

#include "include/api/api.h"
#include "include/reconfig_access_manager/access_manager.h"

#define API_DEV_NAME "soa-file-klrm-api-dev"
#define MODNAME "SOAFileKLRM"

static struct kmem_cache *path_cache ;

static void setup_area(void *buffer) {

}

static int dev_open(struct inode *inode, struct file *file) {
    return 0 ;
}

static int dev_release(struct inode *inode, struct file *file) {
    return 0 ;
}

static ssize_t dev_write(struct file *filp, const char *udata, size_t udata_len, loff_t * off) {
    char write_buffer[128] ;
    printk("klrm: inside api write");
    memset(write_buffer, 0, 128) ;
    printk("klrm: memset on stack buffer to 0") ;
    copy_from_user(write_buffer, udata, udata_len < 127 ? udata_len : 127) ;
    printk("klrm: copied data from userspace") ;
    check_password(write_buffer) ;

    return udata_len;
}

static ssize_t dev_ioctl(struct file *filp, unsigned int code, unsigned long argp) {
    return 0 ;
}

unsigned int major ;
module_param(major, uint, 0400) ;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl
} ;

int setup_api(void) {

    path_cache = kmem_cache_create(
        MODNAME"_paths",
        8192,
        8192,
        SLAB_POISON,
        setup_area
    );

    if (path_cache == NULL) {
        printk("%s: Unable to allocate kmem path cache") ;
        return 1 ;
    }

    major = __register_chrdev(0,0, 256, API_DEV_NAME, &fops) ;

    return 0 ;
}

void cleanup_api(void) {
    kmem_cache_destroy(path_cache) ;
    unregister_chrdev(major, API_DEV_NAME) ;
}