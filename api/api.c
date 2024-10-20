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

#include "../include/api/api.h"

#define API_DEV_NAME "soa-file-klrm-api-dev"
#define MODNAME "SOAFileKLRM"


static int dev_open(struct inode *inode, struct file *file) {
    return 0 ;
}

static int dev_release(struct inode *inode, struct file *file) {
    return 0 ;
}

static int Major ;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release
} ;

static struct dentry node_dentry ;
struct dentry *dev_dentry ;

int setup_api(void) {
    Major = __register_chrdev(0,0, 256, API_DEV_NAME, &fops) ;

    node_dentry.d_name.name = API_DEV_NAME ;
    node_dentry.d_name.len = strlen(API_DEV_NAME) ;
//    node_dentry.d_name.len = strlen(API_DEV_NAME) ;
//
//    struct qstr name ;
//    name.name = "/dev" ;
//    name.len = 4 ;
//    dev_dentry = d_lookup(current->fs->root.dentry, &name) ;
//    if(IS_ERR(dev_dentry)) {
//        printk(KERN_ERR, "Error opening dentry, details: %d", PTR_ERR(dev_dentry)) ;
//        return 1 ;
//    }
//
//    return vfs_mknod(dev_dentry->d_inode, &node_dentry, O_WRONLY | S_IFCHR, MKDEV(Major,0)) ;
    return 0;
}

void cleanup_api(void) {
    unregister_chrdev(Major, API_DEV_NAME) ;
    //vfs_unlink(dev_dentry->d_inode, &node_dentry, NULL) ;
}