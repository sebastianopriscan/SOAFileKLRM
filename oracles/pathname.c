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
#include <linux/cdev.h>

#include "include/oracles/oracles.h"

path_decree *pathname_oracle(char *path) {
    struct file * file ;
    path_decree *retval ;

    retval = kzalloc(8192, GFP_KERNEL) ;
    if (retval == NULL) {
        printk("SOAFileKLRM : Unable to retrieve pathname for a file") ;
        return NULL ;
    }

    file = filp_open(path, O_RDWR | O_PATH, 0) ;
    if (IS_ERR(file)) {
        printk("SOAFileKLRM : Unable to retrieve file") ;
        kfree(retval) ;
        return NULL ;
    }
    path_get(&file->f_path) ;
    inode_lock_shared(file->f_inode) ;
    down_read(&file->f_inode->i_sb->s_umount) ;
    retval->device = file->f_inode->i_sb->s_dev ;
    retval->inode = file->f_inode->i_ino ;
    retval->path = d_path(&file->f_path, (char *)retval + sizeof(path_decree), 8192 - sizeof(path_decree)) ;
    up_read(&file->f_inode->i_sb->s_umount) ;
    inode_unlock_shared(file->f_inode) ;
    path_put(&file->f_path) ;
    
    retval->file = file ;

    if (IS_ERR(retval->path)) {
        printk("SOAFileKLRM : Unable to translate path") ;
        filp_close(file, NULL) ;
        kfree(retval) ;
        return NULL;
    }

    return retval ;
}

path_decree *pathname_oracle_at(int dirfd, char *path) {

    struct file *dir ;
    struct path *atPath ;
    path_decree *dirpath, *retval;
    char *collation ;
    unsigned int pathindex = 0;

    if (dirfd == AT_FDCWD || path[0] == '/') {
        return pathname_oracle(path) ;
    }

    dirpath = kzalloc(8192, GFP_KERNEL) ;
    if (dirpath == NULL) {
        printk("SOAFileKLRM : Unable retrieve pathname for a file") ;
        return NULL ;
    }
    collation = kzalloc(8192, GFP_KERNEL) ;
    if (collation == NULL) {
        kfree(dirpath) ;
        printk("SOAFileKLRM : Unable retrieve pathname for a file") ;
        return NULL ;
    }

    spin_lock(&current->files->file_lock) ;
    dir = files_lookup_fd_locked(current->files, dirfd);
    if (!dir) {
        spin_unlock(&current->files->file_lock);
        kfree(collation) ;
        kfree(dirpath) ;
        return NULL;
    }

    atPath =&dir->f_path ;
    path_get(atPath) ;
    spin_unlock(&current->files->file_lock);
    dirpath->path = d_path(atPath, (char *)dirpath + sizeof(path_decree), 8192 - sizeof(path_decree)) ;
    path_put(atPath) ;

    if (IS_ERR(dirpath->path)) {
        kfree(collation) ;
        kfree(dirpath) ;
        return NULL;
    }

    memcpy(collation, dirpath->path, strlen(dirpath->path)) ;
    pathindex = strlen(dirpath->path) ;
    if (dirpath->path[strlen(dirpath->path)-1] != '/') {
        collation[pathindex++] = '/' ; 
    }
    memcpy(collation + pathindex, path, strlen(path)) ;

    kfree(dirpath) ;

    retval = pathname_oracle(collation) ;

    kfree(collation) ;

    return retval ;
}