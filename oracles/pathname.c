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
#include <linux/namei.h>

#include "include/oracles/oracles.h"

path_decree *pathname_oracle(char *path) {
    path_decree *retval ;
    struct inode *inode_solved ;
    struct path *path_buf ;

    retval = kzalloc(8192, GFP_KERNEL) ;
    if (retval == NULL) {
        printk("SOAFileKLRM : Unable to retrieve pathname for a file") ;
        return NULL ;
    }

    path_buf = &retval->path_struct ;
    kern_path(path, 0, path_buf) ;

    if (path_buf->dentry == NULL) {
        kfree(retval) ;
        printk("SOAFileKLRM : Unable to retrieve pathname for a file") ;
        return NULL ;
    }

    path_get(path_buf) ;
    dget(path_buf->dentry) ;

    inode_solved = d_inode(path_buf->dentry) ;
    if (path_buf->dentry->d_inode == NULL) {
        dput(path_buf->dentry) ;
        path_put(path_buf) ;
        kfree(retval) ;
        printk("SOAFileKLRM : Unable to retrieve pathname for a file") ;
        return NULL ;
    }

    inode_lock_shared(path_buf->dentry->d_inode) ;
    down_read(&path_buf->dentry->d_inode->i_sb->s_umount) ;
    retval->device = path_buf->dentry->d_inode->i_sb->s_dev ;
    retval->inode = path_buf->dentry->d_inode->i_ino ;
    retval->path = d_path(path_buf, (char *)retval + sizeof(path_decree), 8192 - sizeof(path_decree)) ;
    up_read(&path_buf->dentry->d_inode->i_sb->s_umount) ;
    inode_unlock_shared(path_buf->dentry->d_inode) ;
    dput(path_buf->dentry) ;
    path_put(path_buf) ;

    if (IS_ERR(retval->path)) {
        dput(path_buf->dentry) ;
        path_put(path_buf) ;
        printk("SOAFileKLRM : Unable to translate path") ;
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