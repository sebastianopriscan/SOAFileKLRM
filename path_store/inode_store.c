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

#include "include/path_store/path_store.h"
#include "store_internal.h"

#define MODNAME "SOAFileKLRM"

static struct kmem_cache *fs_cache ;
static struct kmem_cache *inodes_cache ;

struct list_head fs_stores ;


static inline int inode_allocate(store_fs * store, unsigned long inode, store_entry *se) {
    inode_ht *node = kmem_cache_alloc(inodes_cache, GFP_KERNEL) ;
    if (node == NULL) {
        printk("%s : Error allocating new node, aborting add", MODNAME) ;
        write_unlock(&store_lock) ;
        return 0 ;
    }
    node->num = inode ;
    node->related_entry = se ;
    se->inode = node ;
    list_add(&node->peers, &(store->inodes[inode % HASH_TABLE_SIZE])) ;
    write_unlock(&store_lock) ;
    return 1 ;
}

static int insert_inode_ht(dev_t table, unsigned long inode, store_entry *se) {

    struct list_head *tmp ;
    store_fs *store ;
    write_lock(&store_lock) ;
    list_for_each(tmp, &fs_stores) {
        store = container_of(tmp, store_fs, stores) ;
        if (store->numbers == table) {
            return inode_allocate(store, inode, se) ;
        }
    }

    store = kmem_cache_alloc(fs_cache, GFP_KERNEL) ;
    if (store == NULL) {
        printk("%s : Error allocating new fs cache, aborting add", MODNAME) ;
        write_unlock(&store_lock) ;
        return 0 ;
    }
    list_add(&store->stores, &fs_stores) ;
    return inode_allocate(store, inode, se) ;    
}

static int check_inode(dev_t table, unsigned long inode) {
    read_lock(&store_lock) ;

    struct list_head *tmp, *tmp2 ;
    store_fs *store ;
    inode_ht *node ;
    list_for_each(tmp, &fs_stores) {
        store = container_of(tmp, store_fs, stores) ;
        if (store->numbers == table) {
            list_for_each(tmp2, &store->inodes[inode % HASH_TABLE_SIZE]) {
                node = container_of(tmp2, inode_ht, peers) ;
                if (node->num == inode) {
                    read_unlock(&store_lock) ;
                    return 1 ;
                }
            }
        }
    }
    read_unlock(&store_lock) ;
    return 0 ;
}

static void setup_area(void *buffer) {

}

int setup_inode_store(void) {

    fs_cache = kmem_cache_create("SOAFileKLRM-fs", sizeof(store_fs), 0, SLAB_POISON, setup_area) ;
    if (fs_cache == NULL) {
        printk("%s : Error setupping fs cache") ;
        return 1 ;
    }

    inodes_cache = kmem_cache_create("SOAFileKLRM-ino", sizeof(inode_ht), 0, SLAB_POISON, setup_area) ;
    if (inodes_cache == NULL) {
        kmem_cache_destroy(fs_cache) ;
        printk("%s : Error setupping inode cache") ;
        return 1 ;
    }

    return 0 ;
}


void cleanup_inode_store(void) {
}