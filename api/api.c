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
#include "include/logfs/write_handle.h"
#include "include/oracles/oracles.h"
#include "include/logger/logger.h"

#define API_DEV_NAME "soa-file-klrm-api-dev"
#define MODNAME "SOAFileKLRM"

#define CODE_MASK 0xc0000000U
#define CHECK_PATH 0x40000000

extern ssize_t klrm_path_check(klrm_input *input) ;

static struct kmem_cache *input_cache ;
static struct filename *(*getname_kernel_addr)(char *) ;

static void setup_area(void *buffer) {

}

static int dev_open(struct inode *inode, struct file *file) {
    return 0 ;
}

static int dev_release(struct inode *inode, struct file *file) {
    return 0 ;
}

static ssize_t dev_write(struct file *filp, const char *udata, size_t udata_len, loff_t * off) {
    path_decree *decree ;
    char write_buffer[128] ;
    printk("klrm: inside api write");
    memset(write_buffer, 0, 128) ;
    printk("klrm: memset on stack buffer to 0") ;
    copy_from_user(write_buffer, udata, udata_len < 127 ? udata_len : 127) ;
    printk("klrm: copied data from userspace") ;
    //check_password(write_buffer) ;
    //internal_logfilefs_write(write_buffer) ;

    struct file *file = filp_open(write_buffer, O_PATH, 0) ;
    struct list_head *tmp ;

    struct dentry *dir = file->f_path.dentry ;

    path_get(&file->f_path) ;
    printk("klrm: Got path") ;
    inode_lock(file->f_inode) ;
    printk("klrm: Got inode") ;
    down_read(&file->f_inode->i_sb->s_umount) ;
    printk("klrm: Got superblock") ;
    list_for_each(tmp, &dir->d_subdirs) {
        struct dentry *curr = container_of(tmp, struct dentry, d_child) ;

        dget(curr) ;
        inode_lock(curr->d_inode) ;

        printk("SOAFileKLRM : iterating over %s", curr->d_name.name) ;

        dput(curr) ;
        inode_unlock(curr->d_inode) ;
    }

    up_read(&file->f_inode->i_sb->s_umount) ;
    inode_unlock(file->f_inode) ;
    path_put(&file->f_path) ;
    filp_close(file, NULL) ;

    decree = pathname_oracle(write_buffer) ;
    if (decree != NULL) {
        printk("SOAFileKLRM : resolved %s as %s, with major %d, minor %d and number %d",
            write_buffer, decree->path, MAJOR(decree->device), MINOR(decree->device), decree->inode) ;
        kfree(decree) ;
    }

/*
    if (getname_kernel_addr != NULL) {
        struct filename *name = getname_kernel_addr(write_buffer) ;
        printk("SOAFileKLRM, resolved %s as %s", write_buffer, name->name) ;
    }
*/

    log_append() ;
    return udata_len;
}

static ssize_t dev_ioctl(struct file *filp, unsigned int code, unsigned long argp) {

    unsigned int size ;
    unsigned int copied ;
    ssize_t retval ;
    klrm_input *argp_copied  ;

    argp_copied = kmem_cache_alloc(input_cache, GFP_KERNEL) ; ;
 
    if (argp_copied == NULL) {
        printk("%s: Error allocating buffer for copying", MODNAME) ;
        kmem_cache_free(input_cache, argp_copied) ;
        return -ENOMEM ;
    }

    size = code & ~CODE_MASK ;
    if(size != sizeof(klrm_input)) { 
        printk("%s: Data pointed by argp was not of correct size", MODNAME) ; 
        kmem_cache_free(input_cache, argp_copied) ;
        return 1 ; 
    } 

    copied = copy_from_user(argp_copied, (void *) argp, size) ;
    if (copied != 0) {
        printk("%s: Error, unable to copy all memory from user, copied %d of %d", MODNAME, copied, size) ;
        kmem_cache_free(input_cache, argp_copied) ;
        return -EACCES ;
    }

    printk("%s: Copied data from user buffer", MODNAME) ;

    printk("%s code is %#08x, code & CODE_MASK is %#08x", MODNAME, code, code & CODE_MASK) ;

    switch (code & CODE_MASK) {
        case ADD_PATH :
            retval = klrm_path_add(argp_copied) ;
            kmem_cache_free(input_cache, argp_copied) ;
            return retval ;
        case RM_PATH :
            retval = klrm_path_rm(argp_copied) ;
            kmem_cache_free(input_cache, argp_copied) ;
            return retval ;
        case CHECK_PATH :
            retval = klrm_path_check(argp_copied) ;
            kmem_cache_free(input_cache, argp_copied) ;
            return retval ;
        default :
            printk("%s: Invoked non-existant operation", MODNAME) ;
            return -EOPNOTSUPP ;
    }
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

    input_cache = kmem_cache_create(
        MODNAME"_paths",
        8192,
        8192,
        SLAB_POISON,
        setup_area
    );

    if (input_cache == NULL) {
        printk("%s: Unable to allocate kmem path cache", MODNAME) ;
        return 1 ;
    }

    major = __register_chrdev(0,0, 256, API_DEV_NAME, &fops) ;

    getname_kernel_addr = (struct filename *(*)(char *))addr_kprobe_oracle("getname_kernel") ;

    return 0 ;
}

void cleanup_api(void) {
    unregister_chrdev(major, API_DEV_NAME) ;
}