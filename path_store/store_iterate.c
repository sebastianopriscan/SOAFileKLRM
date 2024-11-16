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

#include "store_internal.h"

typedef struct _internal_stack {
    void *ptr ;
    struct list_head list ;

} internal_stack ;

void store_iterate_add(struct file *file) {

    struct list_head *tmp ;
    struct list_head stack = LIST_HEAD_INIT(stack) ;
    struct dentry *dir = file->f_path.dentry ;
    int insert_result ;

    path_get(&file->f_path) ;
    inode_lock(file->f_inode) ;
    down_read(&file->f_inode->i_sb->s_umount) ;
    insert_inode_ht(dir->d_inode->i_sb->s_dev, dir->d_inode->i_ino) ;

REPLAY :
    list_for_each(tmp, &dir->d_subdirs) {
        struct dentry *curr = container_of(tmp, struct dentry, d_child) ;
        struct inode *curr_ino ;
        internal_stack *stkntry ;

        dget(curr) ;

        curr_ino = d_inode(curr) ;
        if (curr_ino == NULL) {
            dput(curr) ;
            continue ;
        }

        inode_lock(curr->d_inode) ;

        insert_result = insert_inode_ht(curr->d_inode->i_sb->s_dev, curr->d_inode->i_ino) ;

        if(!list_empty(&curr->d_subdirs) && insert_result != -1) {
            stkntry = kmalloc(sizeof(internal_stack), GFP_KERNEL) ;
            stkntry->ptr = curr ;
            list_add(&stkntry->list, &stack) ;
        } else {
            inode_unlock(curr->d_inode) ;
            dput(curr) ;
        }
    }

    inode_unlock(dir->d_inode) ;
    dput(dir) ;

    if(!list_empty(&stack)) {
        internal_stack *entry_stack = container_of(stack.next, internal_stack, list) ;
        dir = entry_stack->ptr ;
        list_del(stack.next) ;
        kfree(entry_stack) ;
        goto REPLAY ;
    }

    up_read(&file->f_inode->i_sb->s_umount) ;
    return ;
}

void store_iterate_rm(struct file *file) {

    struct list_head *tmp ;
    struct list_head stack = LIST_HEAD_INIT(stack) ;
    struct dentry *dir = file->f_path.dentry ;
    int insert_result ;

    path_get(&file->f_path) ;
    inode_lock(file->f_inode) ;
    down_read(&file->f_inode->i_sb->s_umount) ;
    rm_inode_ht(dir->d_inode->i_sb->s_dev, dir->d_inode->i_ino) ;
REPLAY :
    list_for_each(tmp, &dir->d_subdirs) {
        struct dentry *curr = container_of(tmp, struct dentry, d_child) ;
        struct inode *curr_ino ;
        internal_stack *stkntry ;

        dget(curr) ;

        curr_ino = d_inode(curr) ;
        if (curr_ino == NULL) {
            dput(curr) ;
            continue ;
        }

        inode_lock(curr->d_inode) ;

        insert_result = rm_inode_ht(curr->d_inode->i_sb->s_dev, curr->d_inode->i_ino) ;

        if(!list_empty(&curr->d_subdirs) && insert_result != -1) {
            stkntry = kmalloc(sizeof(internal_stack), GFP_KERNEL) ;
            stkntry->ptr = curr ;
            list_add(&stkntry->list, &stack) ;
        } else {
            inode_unlock(curr->d_inode) ;
            dput(curr) ;
        }
    }

    inode_unlock(dir->d_inode) ;
    dput(dir) ;

    if(!list_empty(&stack)) {
        internal_stack *entry_stack = container_of(stack.next, internal_stack, list) ;
        dir = entry_stack->ptr ;
        list_del(stack.next) ;
        kfree(entry_stack) ;
        goto REPLAY ;
    }

    up_read(&file->f_inode->i_sb->s_umount) ;
    return ;
}