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
#include <linux/namei.h>

#include "store_internal.h"

typedef struct _internal_stack {
    void *ptr ;
    struct list_head list ;

} internal_stack ;

void store_iterate_add(struct path *path) {

    struct list_head *tmp ;
    struct list_head stack = LIST_HEAD_INIT(stack) ;
    struct dentry *dir = path->dentry ;
    struct inode *ino_res ;
    int insert_result ;

    path_get(path) ;
    dget(dir) ;
    ino_res = d_inode(dir) ;
    if(ino_res == NULL) {
        dput(dir) ;
        path_put(path) ;
        printk("SOAFileKLRM : Store iterate got a spurious path") ;
        return ;
    }
    inode_lock_shared(path->dentry->d_inode) ;
    insert_result = insert_inode_ht(dir->d_inode->i_sb->s_dev, dir->d_inode->i_ino) ;
    if(insert_result == -1) {
        inode_unlock_shared(dir->d_inode) ;
        dput(dir) ;
        path_put(path) ;
        return ;
    }


REPLAY :

    printk("KLRM_ITERATE : iterating over %s", dir->d_name.name) ;
    if(d_is_symlink(dir)) {
        struct inode *curr_ino ;
        internal_stack *stkntry = NULL;
        curr_ino = d_inode(dir) ;
        if (curr_ino != NULL) {
            char *link = NULL , *pth = NULL ;
            stkntry = kmalloc(sizeof(internal_stack), GFP_KERNEL) ;
            printk("KLRM_ITERATE : stkentry is %px", stkntry) ;
            if(!IS_ERR(stkntry)) {
                link = kmalloc(8192, GFP_KERNEL) ;
                printk("KLRM_ITERATE : link is %px", link) ;
                if (!IS_ERR(link)) {
                    pth = dentry_path_raw(dir, link, 8192) ;
                    printk("KLRM_ITERATE : pth is %px", pth) ;
                    if(!IS_ERR(pth)) {
                        printk("KLRM_ITERATE : pth as string is %s", pth) ;
                        struct path resolved ;
                        printk("SOAFileKLRM : entering kern_path") ;
                        kern_path(pth, LOOKUP_FOLLOW , &resolved) ;
                        printk("SOAFileKLRM : Exited kern_path") ;
                        if (resolved.dentry != NULL) {
                            struct inode *res_ino ;
                            dget(resolved.dentry) ;
                            printk("SOAFileKLRM : Resolved dentry") ;
                            res_ino = d_inode(resolved.dentry) ;
                            if (res_ino != NULL) {
                                int insert_result ;
                                printk("SOAFileKLRM : Resolved inode") ;
                                inode_lock_shared(resolved.dentry->d_inode) ;
                                insert_result = insert_inode_ht(resolved.dentry->d_inode->i_sb->s_dev,
                                    resolved.dentry->d_inode->i_ino) ;
                                
                                if (insert_result != -1) {
                                    stkntry->ptr = resolved.dentry;
                                    list_add(&stkntry->list, &stack) ;
                                } else {
                                    kfree(stkntry) ;
                                    inode_unlock_shared(resolved.dentry->d_inode) ;
                                    dput(resolved.dentry) ;
                                }

                            } else {
                                dput(resolved.dentry) ;
                            }
                        } else {
                            kfree(stkntry) ;
                        }
                    } else {
                        kfree(stkntry) ;
                    }
                    kfree(link) ;
                } else {
                    kfree(stkntry) ;
                }
            }
        }
    } else {
        list_for_each(tmp, &dir->d_subdirs) {
            struct dentry *curr = container_of(tmp, struct dentry, d_child) ;
            struct inode *curr_ino ;
            internal_stack *stkntry ;

            dget(curr) ;

            printk("KLRM_ITERATE : parent : %s, child %s", dir->d_name.name, curr->d_name.name) ;

            curr_ino = d_inode(curr) ;
            if (curr_ino == NULL) {
                dput(curr) ;
                continue ;
            }
            
            printk("KLRM_ITERATE : solved inode") ;

            inode_lock_shared(curr->d_inode) ;

            insert_result = insert_inode_ht(curr->d_inode->i_sb->s_dev, curr->d_inode->i_ino) ;

            if(insert_result != -1) {
                stkntry = kmalloc(sizeof(internal_stack), GFP_KERNEL) ;
                stkntry->ptr = curr ;
                list_add(&stkntry->list, &stack) ;
            } else {
                inode_unlock_shared(curr->d_inode) ;
                dput(curr) ;
            }
        }
    }

    inode_unlock_shared(dir->d_inode) ;
    dput(dir) ;

    if(!list_empty(&stack)) {
        internal_stack *entry_stack = container_of(stack.next, internal_stack, list) ;
        dir = entry_stack->ptr ;
        list_del(stack.next) ;
        kfree(entry_stack) ;
        goto REPLAY ;
    }

    return ;
}

void store_iterate_rm(struct path *path) {

    struct list_head *tmp ;
    struct list_head stack = LIST_HEAD_INIT(stack) ;
    struct dentry *dir = path->dentry ;
    struct inode *ino_res ;
    int insert_result ;

    path_get(path) ;
    dget(dir) ;
    ino_res = d_inode(dir) ;
    if(ino_res == NULL) {
        dput(dir) ;
        path_put(path) ;
        printk("SOAFileKLRM : Store iterate got a spurious path") ;
        return ;
    }
    inode_lock_shared(path->dentry->d_inode) ;
    insert_result = rm_inode_ht(dir->d_inode->i_sb->s_dev, dir->d_inode->i_ino) ;
    if(insert_result == -1) {
        inode_unlock_shared(dir->d_inode) ;
        dput(dir) ;
        path_put(path) ;
        return ;
    }

REPLAY :

    if(d_is_symlink(dir)) {
        struct inode *curr_ino ;
        internal_stack *stkntry ;
        curr_ino = d_inode(dir) ;
        if (curr_ino != NULL) {
            char *link , *pth;
            stkntry = kmalloc(sizeof(internal_stack), GFP_KERNEL) ;
            if(!IS_ERR(stkntry)) {
                link = kmalloc(8192, GFP_KERNEL) ;
                if (!IS_ERR(link)) {
                    pth = dentry_path_raw(dir, link, 8192) ;
                    if(!IS_ERR(pth)) {
                        struct path resolved ;
                        printk("SOAFileKLRM : entering kern_path") ;
                        kern_path(pth, LOOKUP_FOLLOW , &resolved) ;
                        printk("SOAFileKLRM : Exited kern_path") ;
                        if (resolved.dentry != NULL) {
                            struct inode *res_ino ;
                            dget(resolved.dentry) ;
                            res_ino = d_inode(resolved.dentry) ;
                            if (res_ino != NULL) {
                                inode_lock_shared(res_ino) ;
                                stkntry->ptr = resolved.dentry;
                                list_add(&stkntry->list, &stack) ;
                            } else {
                                dput(resolved.dentry) ;
                            }
                        }
                    }
                    kfree(stkntry) ;
                    kfree(link) ;
                } else {
                    kfree(stkntry) ;
                }
            }
        }
    } else {
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

            inode_lock_shared(curr->d_inode) ;

            insert_result = rm_inode_ht(curr->d_inode->i_sb->s_dev, curr->d_inode->i_ino) ;

            if(!list_empty(&curr->d_subdirs) && insert_result != -1) {
                stkntry = kmalloc(sizeof(internal_stack), GFP_KERNEL) ;
                stkntry->ptr = curr ;
                list_add(&stkntry->list, &stack) ;
            } else {
                inode_unlock_shared(curr->d_inode) ;
                dput(curr) ;
            }
        }
    }

    inode_unlock_shared(dir->d_inode) ;
    dput(dir) ;

    if(!list_empty(&stack)) {
        internal_stack *entry_stack = container_of(stack.next, internal_stack, list) ;
        dir = entry_stack->ptr ;
        list_del(stack.next) ;
        kfree(entry_stack) ;
        goto REPLAY ;
    }

    return ;
}